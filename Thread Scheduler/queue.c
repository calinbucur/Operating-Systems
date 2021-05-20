#include "queue.h"

// Initializes the queue
queue* init_q(void)
{
	// Allocate the queue
	queue *q = malloc(sizeof(queue));

	if (!q)
		exit(-1);
	// Allocate the array with an initial capacity of 2;
	q->cap = 2;
	q->arr = (thread **)malloc(q->cap * sizeof(thread *));
	if (!q->arr)
		exit(-1);
	q->size = 0; // Initially the queue is empty
	q->crt_age = 0; // Initilly the age is zero
	return q; // Return the queue
}

// Insert the thread in a queue (not a priority queue, not even FIFO)
void ins_q(queue *q, thread *t)
{
	// If there is space, simply add the queue at the end and increment the size
	if (q->size < q->cap) {
		q->arr[q->size++] = t;
	} else {
		// If the queue is full, double its capacity and insert the thread
		thread **aux = malloc(2 * q->cap * sizeof(thread *));

		if (!aux)
			exit(-1);
		memcpy(aux, q->arr, q->cap * sizeof(thread *));
		q->cap *= 2;
		free(q->arr);
		q->arr = aux;
		q->arr[q->size++] = t;
	}
}

// Extract the first element from a queue
thread *extr_q(queue *q)
{
	thread *t = q->arr[0];

	q->size--;
	// Replace the first element with the last
	// Just because it's similar with the heap function
	// Doesn't respect FIFO, but doesn't need to
	q->arr[0] = q->arr[q->size];
	return t;
}

// Get the parent node index from the heap
int parent(int i)
{
	return (i - 1) / 2;
}

// Get the left child index from the heap
int left(int i)
{
	return (2 * i + 1);
}

// Get the right child index from the heap
int right(int i)
{
	return (2 * i + 2);
}

// Return the first element from the heap (the maximum) without extracting it
thread *getMax(queue *q)
{
	return q->arr[0];
}

// Swap two nodes in the heap
void swap(thread **x, thread **y)
{
	thread *temp = *x;

	*x = *y;
	*y = temp;
}

// Insert in the priority queue (heap)
void ins_pq(queue *q, thread *t)
{
	t->age = q->crt_age++; // Set the current thread as the newest yet
	// If the queue id full, double the capacity
	if (q->size == q->cap) {
		thread **aux = malloc(2 * q->cap * sizeof(thread *));

		if (!aux)
			exit(-1);
		memcpy(aux, q->arr, q->cap * sizeof(thread *));
		q->cap *= 2;
		free(q->arr);
		q->arr = aux;
	}
	int i = q->size++;

	// Insert the thread at the end
	q->arr[i] = t;
	// Move it up in the heap until it respects the heap property
	while ((i != 0) && (q->arr[parent(i)]->prio < q->arr[i]->prio)) {
		swap(&q->arr[i], &q->arr[parent(i)]);
		i = parent(i);
	}
}

// Check if the queue is empty
int isEmpty(queue *q)
{
	return !q->size;
}

// Rearrange the heap starting from index i in order to maintain the heap property
void max_heapify(queue *q, int i)
{
	int l = left(i);
	int r = right(i);
	int greatest = i;

	// Order the threads by priority
	// If the priorities are equal order them by age in order to preserve FIFO
	if (l < q->size && (q->arr[l]->prio > q->arr[i]->prio || (q->arr[l]->prio == q->arr[i]->prio && q->arr[l]->age < q->arr[i]->age)))
		greatest = l;
	if (r < q->size && (q->arr[r]->prio > q->arr[greatest]->prio || (q->arr[r]->prio == q->arr[greatest]->prio && q->arr[r]->age < q->arr[greatest]->age)))
		greatest = r;
	if (greatest != i) {
		swap(&q->arr[i], &q->arr[greatest]);
		max_heapify(q, greatest);
	}
}

// Extract the first element from the priority queue (heap)
thread *extr_pq(queue *q)
{
	if (q->size == 1) {
		q->size--;
		return q->arr[0];
	}
	thread *t = q->arr[0];

	q->arr[0] = q->arr[q->size-1];
	q->size--;
	// Maintain the heap property
	max_heapify(q, 0);
	return t;
}

// Destroy the queue and free everything
void destr_q(queue *q)
{
	free(q->arr);
	free(q);
}
