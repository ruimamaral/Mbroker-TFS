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
#define SUBSCRIBER_RESPONSE_SIZE (sizeof(uint8_t) \
		+ MAX_MSG_LENGTH * sizeof(char))

#define BOX_NAME_LENGTH 32
#define CLIENT_PIPE_LENGTH 256

#define REQUEST_WBOX_SIZE (sizeof(uint8_t) + CLIENT_PIPE_LENGTH \
		* sizeof(char) + BOX_NAME_LENGTH * sizeof(char))
#define REQUEST_NO_BOX_SIZE (sizeof(uint8_t) + MAX_MSG_LENGTH * sizeof(char))

#define ERROR_MSG_LEN 1024

#define ERR_BOX_EXISTS "Box already exists!"
#define ERR_BOX_DOESNT_EXIST "Box does not exist!"

#define ERR_TOO_MANY_BOXES "Max amount of boxes reached!"
#define ERR_TFS_FAIL "Could not create file in tfs!"

#define MANAGER_BOX_CREATE "create"
#define MANAGER_BOX_REMOVE "remove"
#define MANAGER_BOX_LIST "list"

#define MANAGER_CREATE_CODE 3
#define MANAGER_CREATE_RESPONSE_CODE 4

#define MANAGER_REMOVE_CODE 5
#define MANAGER_REMOVE_RESPONSE_CODE 6

#define MANAGER_LIST_CODE 7
#define MANAGER_LIST_RESPONSE_CODE 8


#define MANAGER_RESPONSE_SIZE (sizeof(char) * ERROR_MSG_LEN \
		+ sizeof(int32_t) + sizeof(uint8_t))

#define MANAGER_LIST_RESPOND_SIZE (sizeof(uint8_t) * 2 \
		+ sizeof(char)*MAX_BOX_NAME + sizeof(uint64_t)*3)

#define SET_ERROR(buf, msg, code) { \
	memcpy(buf, msg, strlen(msg) * sizeof(char)); \
	code = -1; \
}

void requestcpy(void *request,size_t *request_offset, void *data, size_t size);

ssize_t write_pipe(int fd, void const *buffer,size_t buffer_size);

ssize_t read_pipe(int fd, void const *buffer,size_t buffer_size);

ssize_t send_request(int fd, void* buffer,size_t buffer_size);

void *myalloc(size_t size);

#endif