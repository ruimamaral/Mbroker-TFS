#include "data.h"
#include "producer-consumer.h"
#include "locks.h"
#include "operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

box_t **server_boxes;
pc_queue_t *queue;
pthread_mutex_t box_table_lock;
pthread_mutex_t *box_locks;

int data_init() {
	server_boxes = (box_t**) myalloc(sizeof(box_t*) * DEFAULT_BOX_LIMIT);
	queue = (pc_queue_t*) myalloc(sizeof(pc_queue_t*));
	box_locks = (pthread_mutex_t*) myalloc(
			sizeof(pthread_mutex_t) * DEFAULT_BOX_LIMIT);
	for (int i = 0; i < DEFAULT_BOX_LIMIT; i++) {
		mutex_init(&box_locks[i]);
	}
	pcq_create(queue, DEFAULT_QUEUE_LENGTH);
	memset(server_boxes, 0, DEFAULT_BOX_LIMIT * sizeof(char));
	mutex_init(&box_table_lock);
	return 0;
}

box_t *fetch_box(char *name) {
	box_t *box;
	mutex_lock(&box_table_lock);
	int index = find_box(name);
	if (index < 0) {
		return 0;
	}
	box = server_boxes[index];
	mutex_unlock(&box_table_lock);
	return box;
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

int create_box(char *box_name) {
	int i;
	int slot = -1;
	box_t* box;
	mutex_lock(&box_table_lock);
	for (i = 0; i < DEFAULT_BOX_LIMIT; i++) {
		box_t *box = server_boxes[i];
		if (box && !strcmp(box->name, box_name)) {
			// Box already exists
			return -1;
		}
		if (!box && slot < 0) {
			// First empty slot
			slot = i;
		}
	}
	if (slot < 0) {
		// No space
		return -1;
	}
	mutex_lock(&box_locks[slot]);
	box = (box_t*) myalloc(sizeof(box_t));
	if (!box_alloc(box, box_name)) {
		// Could not allocate box
		return -1;
	}
	server_boxes[slot] = box;
	mutex_unlock(&box_locks[slot]);
	mutex_unlock(&box_table_lock);
	return 0;
}

int box_remove(char* box_name) {
	box_t *box;
	int i;
	mutex_lock(&box_table_lock);
	mutex_lock(&box_locks[i]);
	i = find_box(box_name);
	
	if (i < 0) {
		return -1;
	}
	box = server_boxes[i];
	server_boxes[i] = NULL;
	mutex_unlock(&box_locks[i]);
	mutex_unlock(&box_table_lock);
	box_kill(box);
	free(box);

	return 0;
}

int box_alloc(box_t *box, char *box_name) {
	int ret;
	if ((ret = tfs_open(box_name, TFS_O_CREAT)) == -1) {
		return -1;
	}
	tfs_close(ret); // close file we just opened
	mutex_init(&box->content_mutex);
	mutex_init(&box->condvar_mutex);
	cond_init(&box->condvar);

	box->path = (char*) myalloc(sizeof(char) * TFS_BOX_PATH_LEN);
	memset(box->path, 0, TFS_BOX_PATH_LEN * sizeof(char));
	box->path[0] = "/";
	box->name = box->path[1];
	memcpy(box->name, box_name, MAX_BOX_NAME - 1);
	box->n_publishers = 0;
	box->n_subscribers = 0;
	//box->status = OPEN;

	return 0;
}

void box_kill(box_t* box) {
	mutex_lock(&box->content_mutex);
	free(box->path);
	mutex_unlock(&box->content_mutex);
	mutex_kill(&box->content_mutex);
	mutex_kill(&box->condvar_mutex);
	cond_kill(&box->condvar);
	tfs_unlink(box->path);
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

void data_kill() {
	pcq_destroy(queue);
	free(queue);
}