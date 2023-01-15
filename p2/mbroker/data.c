#include "data.h"
#include "producer-consumer.h"
#include "locks.h"
#include "operations.h"
#include "betterassert.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

box_t **server_boxes;
pc_queue_t *queue;
pthread_mutex_t box_table_lock;
allocation_state_t *free_box_table;
size_t box_amount;

int data_init() {
	server_boxes = (box_t**) myalloc(sizeof(box_t*) * DEFAULT_BOX_LIMIT);
	queue = (pc_queue_t*) myalloc(sizeof(pc_queue_t));
	free_box_table = (allocation_state_t*)
			myalloc(sizeof(allocation_state_t) * DEFAULT_BOX_LIMIT);

	pcq_create(queue, DEFAULT_QUEUE_LENGTH);
	mutex_init(&box_table_lock);
	mutex_lock(&box_table_lock);
	for (int i = 0; i < DEFAULT_BOX_LIMIT; i++) {
		server_boxes[i] = (box_t*) myalloc(sizeof(box_t));
		box_alloc(server_boxes[i]);
		free_box_table[i] = FREE;
	}
	box_amount = 0;
	mutex_unlock(&box_table_lock);
	return 0;
}

box_t *fetch_box(char *name) {
	box_t *box;
	int index = find_box(name);
	if (index < 0) {
		return 0;
	}
	box = server_boxes[index];
	return box;
}

int find_box(char *name) {
	int i;
	for (i = 0; i < DEFAULT_BOX_LIMIT; i++) {
		box_t *box = server_boxes[i];
		if (free_box_table[i] == TAKEN && !strcmp(box->name, name)) {
			return i;
		}
	}
	return -1;
}

int box_create(char *box_name) {
	int i;
	int slot = -1;
	int ret;
	box_t* box;
	mutex_lock(&box_table_lock);
	for (i = 0; i < DEFAULT_BOX_LIMIT; i++) {
		box = server_boxes[i];
		mutex_lock(&box->content_mutex);
		if (free_box_table[i] == TAKEN && !strcmp(box->name, box_name)) {
			// Box already exists
			printf("box is taken(cannot create\n");
			mutex_unlock(&box->content_mutex);
			mutex_unlock(&box_table_lock);
			return -1;
		}
		mutex_unlock(&box->content_mutex);
		if (slot < 0 && free_box_table[i] == FREE) {
			// First empty slot
			printf("box was free\n");
			slot = i;
		}
	}
	printf("second part of box_create\n");
	if (slot < 0) {
		// No space
		printf("there was no slot to create box\n");
		mutex_unlock(&box_table_lock);
		return -2;
	}
	printf("CREATE BEFORE |||| box_path->%s||box_name->%s\n",box->path,box->name);

	box = server_boxes[slot];
	mutex_lock(&box->content_mutex);
	box_init(box);
	memcpy(box->name, box_name, MAX_BOX_NAME * sizeof(char)); 
	// Create file in tfs (or open and truncate contents)
	if ((ret = tfs_open(box->path, TFS_O_CREAT | TFS_O_TRUNC)) == -1) {
		mutex_unlock(&box->content_mutex);
		mutex_unlock(&box_table_lock);
		return -3;
	} 
	tfs_close(ret); // close the file we just opened
	free_box_table[slot] = TAKEN;
	box_amount++;

	printf("CREATE |||| box_path->%s||box_name->%s\n",server_boxes[slot]->path,server_boxes[slot]->name);

	mutex_unlock(&box->content_mutex);
	mutex_unlock(&box_table_lock);
	printf("was able to create\n");
	return 0;
}

int box_remove(char* box_name) {
	box_t *box;
	int i;
	int pub_fd = 0;

	mutex_lock(&box_table_lock);

	i = find_box(box_name);
	if (i < 0) {
		mutex_unlock(&box_table_lock);
		return -1;
	}
	box = server_boxes[i];

	// Lock to prevent signal leakage and/or data races from accessing box.
	mutex_lock(&box->content_mutex);

	// Should never happen during regular execution
	ALWAYS_ASSERT(box->status == NORMAL,
			"Illegal access to box has occurred.");

	// Changes flag to closed to notify any subscribers/publishers
	box->status = CLOSED;
	// If box has a publisher, writes in its pipe in order to unlock it
	// and force it into checking the box status flag
	if (box->n_publishers > 0) {
		pub_fd = open(box->pub_pipe_name, O_WRONLY);
		uint8_t pub_code = PUBLISH_CODE;
		write_pipe(pub_fd, &pub_code, sizeof(uint8_t));
	}

	// ...and then waits until the publisher has shut down completely.
	while (box->n_publishers > 0) {
		// After writing in its pipe, the publisher is going to
		// check the status flag and see that the box was closed so,
		// it will terminate its session and signal the cond.
		cond_wait(&box->condvar, &box->content_mutex);
	}
	mutex_unlock(&box->content_mutex);

	if (pub_fd) {
		// At this point, we know that the publisher has ended its session
		// so we can safely close the pipe
		close(pub_fd);
	}

	// If the box has subscribers, broadcasts the cond to force the
	// subscribers into checking the box status flag
	mutex_lock(&box->content_mutex);
	if (box->n_subscribers > 0) {
		cond_broadcast(&box->condvar);
	}
	mutex_unlock(&box->content_mutex);

	// Lock to prevent signal leakage and/or data races from accessing box.
	mutex_lock(&box->content_mutex);
	// After broadcasting to subscribers, we wait until they have
	// all ended their sessions.
	while (box->n_subscribers > 0) {
		// Once a subscriber gets signaled, it will check the box status
		// flag and see that it has been closed, so it will end its session
		// and then signal the cond.
		cond_wait(&box->condvar, &box->content_mutex);
	}

	// After all client sessions are terminated, we can safely
	// formally remove the box from the server.
	tfs_unlink(box->path);
	free_box_table[i] = FREE;
	box_amount--;

	mutex_unlock(&box->content_mutex);
	mutex_unlock(&box_table_lock);

	return 0;
}

int box_init(box_t *box) {

	box->n_publishers = 0;
	box->n_subscribers = 0;
	box->box_size = 0;
	box->status = NORMAL;

	memset(box->name, 0, MAX_BOX_NAME * sizeof(char));
	memset(box->pub_pipe_name, 0, CLIENT_PIPE_LENGTH * sizeof(char));

	return 0;
}

int box_alloc(box_t *box) {
	mutex_init(&box->content_mutex);
	mutex_lock(&box->content_mutex);
	cond_init(&box->condvar);

	box->path = (char*) myalloc(sizeof(char) * TFS_BOX_PATH_LEN);
	box->pub_pipe_name = (char*) myalloc(sizeof(char) * CLIENT_PIPE_LENGTH);
	memset(box->path, 0, TFS_BOX_PATH_LEN * sizeof(char));
	box->path[0] = '/'; // Add slash for tfs_open
	box->name = box->path + sizeof(char); // name is path without leading '/'

	box_init(box);
	mutex_unlock(&box->content_mutex);
	return 0;
}

box_t **box_get_all(size_t *amount) {
	mutex_lock(&box_table_lock);
	(*amount) = box_amount;
	size_t count = 0;
	if ((*amount) == 0) {
		mutex_unlock(&box_table_lock);
		return 0;
	}

	box_t **boxes = (box_t**) myalloc(sizeof(box_t*) * box_amount);

	for (int i = 0; i < DEFAULT_BOX_LIMIT; i++) {
		if (free_box_table[i] == TAKEN) {
			boxes[count] = (box_t*) myalloc(sizeof(box_t));
			// Copy box contents to array
			(*boxes[count++]) = (*server_boxes[i]);
		}
	}
	mutex_unlock(&box_table_lock);
	// Should never happen during regular execution.
	ALWAYS_ASSERT(count == (*amount), "Sanity check failed!");
	return boxes;
}

box_t *box_add_pub(char *box_name) {
	box_t *box;
	mutex_lock(&box_table_lock);
	if ((box = fetch_box(box_name)) == 0) {
		mutex_unlock(&box_table_lock);
		return 0;
	}
	mutex_lock(&box->content_mutex);
	if (box->n_publishers > 0) {
		mutex_unlock(&box->content_mutex);
		mutex_unlock(&box_table_lock);
		return 0;
	}
	box->n_publishers++;
	mutex_unlock(&box->content_mutex);
	mutex_unlock(&box_table_lock);
	return box;
}

box_t *box_add_sub(char *box_name) {
	box_t *box;
	mutex_lock(&box_table_lock);
	if ((box = fetch_box(box_name)) == 0) {
		mutex_unlock(&box_table_lock);
		return 0;
	}
	mutex_lock(&box->content_mutex);
	box->n_subscribers++;
	mutex_unlock(&box->content_mutex);
	mutex_unlock(&box_table_lock);
	return box;
}

void box_kill(box_t* box) {
	mutex_lock(&box->content_mutex);
	free(box->path);
	free(box->pub_pipe_name);
	cond_kill(&box->condvar);
	mutex_unlock(&box->content_mutex);
	mutex_kill(&box->content_mutex);
}

int data_kill() {
	pcq_destroy(queue);
	free(queue);
	mutex_lock(&box_table_lock);
	for (int i = 0; i < DEFAULT_BOX_LIMIT; i++) {
		box_t *box = server_boxes[i];
		if (free_box_table[i] == TAKEN) {
			// If box was in use, delete its file from tfs
			tfs_unlink(box->path);
		}
		box_kill(box);
		free(box);
	}
	free(free_box_table);
	free(server_boxes);
	mutex_unlock(&box_table_lock);
	mutex_kill(&box_table_lock);
	return 0;
}