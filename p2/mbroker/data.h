#ifndef DATA_H 
#define DATA_H

#include "pipeutils.h"
#include "producer-consumer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DEFAULT_QUEUE_LENGTH 1024
#define DEFAULT_BOX_LIMIT 32

typedef enum { OPEN = 0, CLOSED = 1 } box_status_t;

typedef struct {
	char name[MAX_BOX_NAME];
	pthread_mutex_t content_mutex;
	int n_subscribers;
	int n_publishers;
	pthread_mutex_t condvar_mutex;
	pthread_cond_t condvar;
	char pub_pipe_name[CLIENT_PIPE_LENGTH];
	box_status_t status;
} box_t;

typedef struct {
	uint8_t code;
	char box_name[BOX_NAME_LENGTH];
	char pipe_name[CLIENT_PIPE_LENGTH];
} session_t;



extern box_t **server_boxes;
extern pc_queue_t *queue;

int data_init();
box_t *fetch_box(char *name);
int find_box(char *name);
box_t* create_box(char *name,char* pipe_name);
void decrease_box_subs(box_t* box);
void decrease_box_pubs(box_t* box);

#endif