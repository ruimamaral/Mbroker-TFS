#ifndef PIPEUTILS_H
#define PIPEUTILS_H

#include "logging.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_MSG_LENGTH 1024
#define MAX_BOX_NAME 32
#define PUBLISH_CODE 9
#define PUB_CREATION_CODE 1
#define BOX_NAME_LENGTH 32
#define CLIENT_PIPE_LENGTH 256


void requestcpy(void *request,
		size_t *request_offset, void *data, size_t size);

ssize_t write_pipe(int fd, char const *buf);

ssize_t read_pipe(int fd, char const *buf);

ssize_t send_request(int fd, char* buf);

void *myalloc(size_t size);

#endif