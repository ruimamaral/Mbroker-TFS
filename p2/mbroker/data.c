#include "data.h"
#include "producer-consumer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

box_t **server_boxes;
pc_queue_t *queue;

int data_init() {
	server_boxes = (box_t**) myalloc(sizeof(box_t*) * DEFAULT_BOX_LIMIT);
	queue = (pcq_queue_t*) myalloc(sizeof(pcq_queue_t));
	pcq_create(queue, DEFAULT_QUEUE_LENGTH);
	memset(server_boxes, 0, DEFAULT_BOX_LIMIT);
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
		box_t box* = server_boxes[i];
		if (box && !strcmp(box->name, name)) {
			return i;
		}
	}
	return -1;
}

box_t* create_box(char *name) {
	int i = 0;
}