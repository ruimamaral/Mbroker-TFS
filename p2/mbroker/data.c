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

int data_init() {
	server_boxes = (box_t**) myalloc(sizeof(box_t*) * DEFAULT_BOX_LIMIT);
	queue = (pc_queue_t*) myalloc(sizeof(pc_queue_t));
	pcq_create(queue, DEFAULT_QUEUE_LENGTH);
	memset(server_boxes, 0, DEFAULT_BOX_LIMIT);
	mutex_init(&box_table_lock);
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
		box = server_boxes[i];
		if (box && !strcmp(box->name, box_name)) {
			// Box already exists
			printf("NO->1\n");
			return -1;
		}
		if (!box && slot < 0) {
			// First empty slot
			printf("NO->2\n");
			slot = i;
		}
	}
	printf("EW\n");
	if (slot < 0) {
		// No space
		printf("NO->3\n");
		return -2;
	}
	printf("EW2\n");
	box = (box_t*) myalloc(sizeof(box_t));
	if (box_alloc(box, box_name)) {
		// Could not allocate box
		printf("NO->4\n");
		free(box);
		return -3;
	}
	printf("CREATE BEFORE |||| box_path->%s||box_name->%s\n",box->path,box->name);
	server_boxes[slot] = box;
	printf("CREATE |||| box_path->%s||box_name->%s\n",server_boxes[slot]->path,server_boxes[slot]->name);
	mutex_unlock(&box_table_lock);
	printf("YES\n");
	return 0;
}

int box_remove(char* box_name) {
	box_t *box;
	int i;
	mutex_lock(&box_table_lock);
	i = find_box(box_name);
	if (i < 0) {
		return -1;
	}
	box = server_boxes[i];
	server_boxes[i] = NULL;
	box_kill(box);
	mutex_unlock(&box_table_lock);
	free(box);

	return 0;
}

int box_alloc(box_t *box, char *box_name) {
	printf("AYO\n");
	char dest_path[TFS_BOX_PATH_LEN];
	memcpy(dest_path,"/",sizeof(char));
	memcpy(dest_path+1,box_name,MAX_BOX_NAME-1);
	printf("dest->%s\n",dest_path);
	int ret; 
	if ((ret = tfs_open(dest_path, TFS_O_CREAT & TFS_O_TRUNC)) == -1) {
		return -1;
	} 
	tfs_close(ret); // close the file we just opened
	mutex_init(&box->content_mutex);
	mutex_init(&box->condvar_mutex);
	cond_init(&box->condvar);

	mutex_lock(&box->content_mutex);

	box->path = (char*) myalloc(sizeof(char) * TFS_BOX_PATH_LEN);
	memset(box->path, 0, TFS_BOX_PATH_LEN * sizeof(char));
	memcpy(box->path, dest_path, MAX_BOX_NAME); 
	box->name = box->path + 1;

	box->n_publishers = 0;
	box->n_subscribers = 0;
	//box->status = OPEN;

	mutex_unlock(&box->content_mutex);
	return 0;
}

void box_kill(box_t* box) {
	mutex_lock(&box->content_mutex);
	free(box->path);
	mutex_kill(&box->condvar_mutex);
	cond_kill(&box->condvar);
	tfs_unlink(box->path);
	mutex_unlock(&box->content_mutex);
	mutex_kill(&box->content_mutex);
}

box_t *add_pub_to_box(char *box_name) {
	box_t *box;
	mutex_lock(&box_table_lock);
	if ((box = fetch_box(box_name)) == 0) {
		return 0;
	}
	mutex_lock(&box->content_mutex);
	if (box->n_publishers > 0) {
		mutex_unlock(&box->content_mutex);
		return 0;
	}
	box->n_publishers++;
	mutex_unlock(&box->content_mutex);
	mutex_unlock(&box_table_lock);
	return box;
}

box_t *add_sub_to_box(char *box_name) {
	box_t *box;
	mutex_lock(&box_table_lock);
	if ((box = fetch_box(box_name)) == 0) {
		return 0;
	}
	mutex_lock(&box->content_mutex);
	box->n_subscribers++;
	mutex_unlock(&box->content_mutex);
	mutex_unlock(&box_table_lock);
	return box;
}

/* void data_kill() {
	pcq_destroy(queue);
	free(queue);
} */