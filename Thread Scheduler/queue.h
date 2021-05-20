// Calin Bucur 332CB
// Tema 4
#include "./util/so_scheduler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

// This structure holds all the info needed about a thread
typedef struct {
	tid_t id; // The thread id
	int prio; // The priority of the thread
	int quant; // The time the thread has left for running
	int age; // The "age" of the thread. Used to guarantee the FIFO property for the heap
	so_handler *func; // The function the thread should run
	sem_t sem; // Semaphore used for stopping/starting the thread execution
} thread;

// This structure holds the info of a custom implemented queue
typedef struct {
	thread **arr; // The array where the elementsare stored
	int cap; // The current capacity of the array
	int size; // The current size of the queue
	int crt_age; // The age of the last element. Used to guarantee the FIFO property for the heap.
} queue;

// Function definitions (explained in queue.c)
queue *init_q(void);

void ins_q(queue *q, thread *t);

thread *extr_q(queue *q);

int parent(int i);

int left(int i);

int right(int i);

thread *getMax(queue *q);

void swap(thread **x, thread **y);

void ins_pq(queue *q, thread *t);

int isEmpty(queue *q);

void max_heapify(queue *q, int i);

thread *extr_pq(queue *q);

void destr_q(queue *q);
