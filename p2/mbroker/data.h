#ifndef DATA_H 
#define DATA_H

#include "pipeutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	char name[MAX_BOX_NAME];
	int n_subs;
	int n_publishers;
	pthread_mutex_t condvar_mutex;
	pthread_cond_t condvar;
} box_t;

typedef struct {
	uint8_t code;
	char *box_name;
	char *pipe_name;
} session_t;

#define DEFAULT_QUEUE_LENGTH 1024
#define DEFAULT_BOX_LIMIT 32

extern box_t **server_boxes;
extern pcq_queue_t *queue;

#endif