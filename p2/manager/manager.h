#ifndef MANAGER_H
#define MANAGER_H


#include "logging.h"
#include "betterassert.h"
#include "pipeutils.h"
#include <stdio.h>
#include <stdlib.h>
#include "unistd.h"
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

typedef struct box_node {
	char box_name[MAX_BOX_NAME];
	uint64_t n_subscribers;
	uint64_t n_publishers ;
	uint64_t box_size;
	uint8_t last;
	struct box_node* next;
} box_node_t;

	
#endif
