#ifndef PIPEUTILS_H
#define PIPEUTILS_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_MSG_LENGTH 1024

void requestcpy(void *request,
		size_t *request_offset, void *data, size_t size);

ssize_t write_pipe(int fd, char const *buf);

ssize_t read_pipe(int fd, char const *buf);

ssize_t send_request(int fd, char buf);

void *myalloc(size_t size);

#endif