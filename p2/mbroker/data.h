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


typedef enum { NORMAL = 0, CLOSED = 1 } box_status_t;
typedef enum { FREE = 0, TAKEN = 1 } allocation_state_t;

typedef struct {
	char *path;
	char *name;
	int n_subscribers;
	int n_publishers;
	pthread_cond_t condvar;
	// We only needed one mutex due to the fact that most conditions
	// depended on the content of the box, therefore, one mutex does
	// both jobs easily.
	pthread_mutex_t content_mutex;
	char *pub_pipe_name;
	box_status_t status;
} box_t;

typedef struct {
	uint8_t code;
	char box_name[BOX_NAME_LENGTH];
	char pipe_name[CLIENT_PIPE_LENGTH];
} session_t;

extern pc_queue_t *queue;

int data_init();
int box_init(box_t *box);
box_t *fetch_box(char *name);
int box_remove(char* box_name);
void box_kill(box_t* box);
int find_box(char *name);
int create_box(char *name);
int box_alloc(box_t *box);
box_t *add_sub_to_box(char *box_name);
box_t *add_pub_to_box(char *box_name);

#endif