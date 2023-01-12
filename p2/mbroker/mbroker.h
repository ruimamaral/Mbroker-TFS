#ifndef MBROKER_H
#define MBROKER_H

#include "pipeutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


typedef struct {
	char path[MAX_BOX_NAME];
	int n_subs;
	int n_publishers;
} box;

typedef struct {
	uint8_t code;
	char *box_name;
	char *pipe_name;
} session;


#endif