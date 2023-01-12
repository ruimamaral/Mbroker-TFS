#include "producer-consumer.h"
#include "locks.h"
#include "pipeutils.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// pcq_create: create a queue, with a given (fixed) capacity
//
// Memory: the queue pointer must be previously allocated
// (either on the stack or the heap)
int pcq_create(pc_queue_t *queue, size_t capacity) {
	mutex_init(&queue->pcq_current_size_lock);
	mutex_init(&queue->pcq_head_lock);
	mutex_init(&queue->pcq_tail_lock);
	mutex_init(&queue->pcq_pusher_condvar_lock);
	mutex_init(&queue->pcq_popper_condvar_lock);

	cond_init(&queue->pcq_pusher_condvar);
	cond_init(&queue->pcq_popper_condvar);

	queue->pcq_capacity = capacity;
	queue->pcq_buffer = myalloc(sizeof(void*) * capacity);
	queue->pcq_head = 0;
	queue->pcq_tail = 0;
	queue->pcq_current_size = 0;
	return 0;
}

// pcq_destroy: releases the internal resources of the queue
//
// Memory: does not free the queue pointer itself


/* int pcq_destroy(pc_queue_t *queue) {

}

 */
// pcq_enqueue: insert a new element at the front of the queue
//
// If the queue is full, sleep until the queue has space
int pcq_enqueue(pc_queue_t *queue, void *elem) {
	// Lock does not allow two pushers running at the same time.
	mutex_lock(&queue->pcq_pusher_condvar_lock);
	if (queue->pcq_current_size == queue->pcq_capacity) {
		// Queue is full, so wait unti an element gets popped
		cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_pusher_condvar_lock);
	}
	mutex_lock(&queue->pcq_current_size_lock);
	queue->pcq_current_size++;
	mutex_unlock(&queue->pcq_current_size_lock);

	cond_signal(&queue->pcq_popper_condvar);

	queue->pcq_buffer[queue->pcq_tail] = elem;
	queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;

	mutex_unlock(&queue->pcq_pusher_condvar_lock);
	return 0;
}

// pcq_dequeue: remove an element from the back of the queue
//
// If the queue is empty, sleep until the queue has an element
void *pcq_dequeue(pc_queue_t *queue) {
	// Lock does not allow two poppers running at the same time.
	mutex_lock(&queue->pcq_popper_condvar_lock);
	if (queue->pcq_current_size == 0) {
		// Queue is empty, so wait until an element gets pushed
		cond_wait(&queue->pcq_popper_condvar, &queue->pcq_popper_condvar_lock);
	}
	mutex_lock(&queue->pcq_current_size_lock);
	queue->pcq_current_size--;
	mutex_unlock(&queue->pcq_current_size_lock);

	cond_signal(&queue->pcq_pusher_condvar);

	void *ret = queue->pcq_buffer[queue->pcq_head];
	queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;

	mutex_unlock(&queue->pcq_popper_condvar_lock);
	return ret;
}