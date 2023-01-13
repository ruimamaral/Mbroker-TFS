#ifndef PIPEUTILS_H
#define PIPEUTILS_H

#include "logging.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_MSG_LENGTH 1024
#define MAX_BOX_NAME 32

#define PUBLISH_CODE 9
#define SUB_RECEIVE_CODE 10

#define PUB_CREATION_CODE 1 
#define SUB_CREATION_CODE 2

#define SUBSCRIBER_RESPONSE_CODE 10
#define SUBSCRIBER_RESPONSE_SIZE (sizeof(uint8_t) + MAX_MSG_LENGTH * sizeof(char))

#define BOX_NAME_LENGTH 32
#define CLIENT_PIPE_LENGTH 256

#define REQUEST_PUBLISH_LEN (sizeof(uint8_t) + CLIENT_PIPE_LENGTH * sizeof(char) + BOX_NAME_LENGTH * sizeof(char))
#define REQUEST_SUB_LEN REQUEST_PUBLISH_LEN

#define ERROR_MSG_LEN 1024

#define ERR_BOX_EXISTS "Box already exists!"

#define SET_ERROR(buf, msg, code) { \
	buf = (char*) myalloc(ERROR_MSG_LEN * sizeof(char)); \
	memcpy(buf, msg, ERROR_MSG_LEN * sizeof(char)); \
	code = -1; \
}



void requestcpy(void *request,
		size_t *request_offset, void *data, size_t size);

ssize_t write_pipe(int fd, void const *buffer,size_t buffer_size);

ssize_t read_pipe(int fd, void const *buffer,size_t buffer_size);

ssize_t send_request(int fd, void* buffer,size_t buffer_size);

void *myalloc(size_t size);

#endif