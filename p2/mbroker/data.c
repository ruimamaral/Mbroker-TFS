#include "data.h"
#include "producer-consumer.h"
#include "locks.h"
#include "operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

box_t **server_boxes;
pc_queue_t *queue;

int data_init() {
	server_boxes = (box_t**) myalloc(sizeof(box_t*) * DEFAULT_BOX_LIMIT);
	queue = (pc_queue_t*) myalloc(sizeof(pc_queue_t*));
	pcq_create(queue, DEFAULT_QUEUE_LENGTH);
	memset(server_boxes, 0, DEFAULT_BOX_LIMIT);
	return 0;
}

box_t *fetch_box(char *name) {
	int index = find_box(name);
	if (index < 0) {
		return 0;
	}
	return server_boxes[index];
}

int find_box(char *name) {
	int i;
	for (i = 0; i < DEFAULT_BOX_LIMIT; i++) {
		box_t *box = server_boxes[i];
		if (box && !strcmp(box->name, name)) {
			return i;
		}
	}
	return -1;
}

box_t* box_alloc(char *box_name,char* pipe_name) {
	box_t* box = (box_t*)myalloc(sizeof(box_t*));
	memcpy(box->name,box_name,sizeof(box->name));
	memcpy(box->pub_pipe_name,pipe_name,sizeof(box->pub_pipe_name));
	mutex_init(&box->content_mutex);
	box->n_publishers = 0;
	box->n_subscribers=0;
	mutex_init(&box->condvar_mutex);
	cond_init(&box->condvar);
	box->status = OPEN ;
	server_boxes[]
}

void erase_box(box_t* box) {
	mutex_kill(&box->content_mutex);
	mutex_kill(&box->condvar_mutex);
	cond_kill(&box->condvar);
	free(box);
}

void decrease_box_subs(box_t* box) {
	mutex_lock(&box->content_mutex);
	box->n_subscribers--;
	mutex_unlock(&box->content_mutex);
}

void decrease_box_pubs(box_t* box) {
	mutex_lock(&box->content_mutex);
	box->n_publishers--;
	mutex_unlock(&box->content_mutex);
}