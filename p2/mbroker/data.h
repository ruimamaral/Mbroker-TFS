#ifndef DATA_H 
#define DATA_H

#include "pipeutils.h"
#include "producer-consumer.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DEFAULT_QUEUE_LENGTH 1024
#define DEFAULT_BOX_LIMIT 32
#define TFS_BOX_PATH_LEN (BOX_NAME_LENGTH + 1)

typedef enum { OPEN = 0, CLOSED = 1 } box_status_t;

typedef struct {
	char *path;
	char *name;
	pthread_mutex_t content_mutex;
	int n_subscribers;
	int n_publishers;
	pthread_mutex_t condvar_mutex;
	pthread_cond_t condvar;
	// char *pub_pipe_name;
	// box_status_t status;
} box_t;

typedef struct {
	uint8_t code;
	char box_name[BOX_NAME_LENGTH];
	char pipe_name[CLIENT_PIPE_LENGTH];
} session_t;

extern pc_queue_t *queue;

int data_init();
box_t *fetch_box(char *name);
int find_box(char *name);
int create_box(char *name);
int box_alloc(box_t *box, char *box_name);
void decrease_box_subs(box_t* box);
void decrease_box_pubs(box_t* box);

#endif