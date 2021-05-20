#include "queue.h"

// This structure holds everything the scheduler needs
typedef struct {
	unsigned int quant; // The maximum time a thread is allowed to run uninterrupted
	unsigned int io_count; // The number of IO events available
	int first; // Checks when the first thread has been forked
	thread *running; // The currently running thread
	queue *rdy_q; // The priority queue holding the ready threads
	queue **wait_q; // Array of queues holding the waiting threads (one queue for each event)
	queue *finished_q; // Queue holding the finished threads
	sem_t done; // Checks if the execution is finished and the threads can be joined
} so_scheduler;

so_scheduler *sched; // The scheduler itself

// Preempts the current thread and runs the next (if that's the case)
void schedule(void)
{
	int ret;

	// If no thread is currently running
	if (!sched->running) {
		// If no threads are ready, we are done and can exit
		if (isEmpty(sched->rdy_q)) {
			ret = sem_post(&sched->done);
			if (ret < 0)
				exit(-1);
			return;
		}
		// Run the highest priority thread from the ready queue
		thread *t = extr_pq(sched->rdy_q);

		sched->running = t;
		ret = sem_post(&t->sem);
		if (ret < 0)
			exit(-1);
	} else {
		sched->running->quant--;
		if (!isEmpty(sched->rdy_q)) {
			thread *t = getMax(sched->rdy_q);

			// If the time expired
			if (sched->running->quant == 0) {
				sched->running->quant = sched->quant;
				thread *aux = sched->running;

				// Push the running thread into the ready queue
				ins_pq(sched->rdy_q, sched->running);
				// Set the highest priority thread as running
				sched->running = extr_pq(sched->rdy_q);
				ret = sem_post(&sched->running->sem);
				if (ret < 0)
					exit(-1);
				// Pause the previous thread
				ret = sem_wait(&aux->sem);
				if (ret < 0)
					exit(-1);
			} else if (sched->running->prio < t->prio) { // If a higher priority thread entered the system
				sched->running->quant = sched->quant;
				t = extr_pq(sched->rdy_q);
				thread *aux = sched->running;

				// Set the highest priority thread as running
				sched->running = t;
				// Push the previous thread into the ready queue
				ins_pq(sched->rdy_q, aux);
				// Start the running thread
				ret = sem_post(&sched->running->sem);
				if (ret < 0)
					exit(-1);
				// Pause the previous thread
				ret = sem_wait(&aux->sem);
				if (ret < 0)
					exit(-1);
			}
		} else if (sched->running->quant == 0) // If there are no ready threads and the time expired, reset the time and keep running
			sched->running->quant = sched->quant;
	}
}

// Initializes the scheduler
int so_init(unsigned int time_quantum, unsigned int io)
{
	// Check the parameters
	// Also check if the scheduler is already initialised
	if (sched || !time_quantum || io > SO_MAX_NUM_EVENTS)
		return -1;
	// Allocate the scheduler
	sched = (so_scheduler *)malloc(sizeof(so_scheduler));
	if (!sched)
		return -1;
	sched->quant = time_quantum; // Set the quantum
	sched->io_count = io; // Set the number of IO events
	sched->first = 1;
	// Initialize the queues
	sched->running = NULL;
	sched->rdy_q = init_q();
	sched->finished_q = init_q();
	sched->wait_q = malloc(io * sizeof(queue *));
	for (unsigned int i = 0; i < io; i++)
		sched->wait_q[i] = init_q();
	int ret = sem_init(&sched->done, 0, 1);

	if (ret < 0)
		exit(-1);
	return 0;
};

// Auxiliary function run by threads to ensure synchronisation
static void *start_thread(void *params)
{
	// Takes the thread structure as parameter
	thread *this = (thread *)params;
	// Initially the thread is paused
	int ret = sem_wait(&this->sem);

	if (ret < 0)
		exit(-1);
	// Run the handler
	this->func(this->prio);
	// When done, clear the running thread
	sched->running = NULL;
	// Insert the thread into the finished threads queue
	ins_q(sched->finished_q, this);
	// Call the scheduler to plan the next thread
	schedule();
	return NULL;
}

// Launch a new thread into the system
tid_t so_fork(so_handler *func, unsigned int priority)
{
	int ret;

	// Check the params
	if (priority > SO_MAX_PRIO || !func)
		return INVALID_TID;
	// Check if it's the first fork
	if (sched->first == 1) {
		sched->first = 0;
		// Make the main thread wait for all others
		ret = sem_wait(&sched->done);
		if (ret < 0)
			return INVALID_TID;
	}
	// Allocate and initialize the thread structure
	thread *t = malloc(sizeof(thread));

	if (!t)
		exit(-1);
	t->prio = priority;
	t->func = func;
	t->quant = sched->quant;
	ret = sem_init(&t->sem, 0, 0);
	if (ret < 0)
		return INVALID_TID;
	// Create the thread
	ret = pthread_create(&t->id, NULL, start_thread, t);
	if (ret != 0)
		return INVALID_TID;
	// Insert it into the ready queue
	ins_pq(sched->rdy_q, t);
	// Call the scheduler
	schedule();
	return t->id;
};

// Makes the current thread wait for a certain IO event
int so_wait(unsigned int io)
{
	// Check the param
	if (io >= sched->io_count)
		return -1;
	// Insert the thread into the waiting queue of the IO event
	ins_q(sched->wait_q[io], sched->running);
	thread *aux = sched->running;

	// Clear the running thread
	sched->running = NULL;
	// Call the scheduler to plan the next thread
	schedule();
	// Pause the waiting thread
	int ret = sem_wait(&aux->sem);

	if (ret < 0)
		exit(-1);
	return 0;
};

// Signals and starts all threads waiting for a certain IO event
int so_signal(unsigned int io)
{
	// Check param
	if (io >= sched->io_count)
		return -1;
	int count = 0;

	// Push every thread waiting for the event into the ready queue
	while (!isEmpty(sched->wait_q[io])) {
		thread *t = extr_q(sched->wait_q[io]);

		ins_pq(sched->rdy_q, t);
		count++;
	}
	// Call the scheduler
	schedule();
	// Return the number of awoken threads
	return count;
};

// Simply calls the scheduler
void so_exec(void)
{
	schedule();
};

// Ends the current instance of the scheduler
void so_end(void)
{
	// Check if the scheduler is valid
	if (!sched)
		return;
	// Wait for all the threads to finish
	int ret = sem_wait(&sched->done);

	if (ret < 0)
		exit(-1);
	// Join every finished thread and free the structure
	for (int i = 0; i < sched->finished_q->size; i++) {
		ret = pthread_join(sched->finished_q->arr[i]->id, NULL);
		if (ret < 0)
			exit(-1);
		ret = sem_destroy(&sched->finished_q->arr[i]->sem);
		if (ret < 0)
			exit(-1);
		free(sched->finished_q->arr[i]);
	}
	// Destroy the queues
	destr_q(sched->finished_q);
	destr_q(sched->rdy_q);
	for (unsigned int i = 0; i < sched->io_count; i++)
		destr_q(sched->wait_q[i]);
	free(sched->wait_q);
	ret = sem_destroy(&sched->done);
	if (ret < 0)
		exit(-1);
	// Free the scheduler
	free(sched);
	sched = NULL;
};
