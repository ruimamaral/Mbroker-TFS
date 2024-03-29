#include "betterassert.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

void requestcpy(void *request,
		size_t *request_offset, void *data, size_t size) {

	memcpy(request + *request_offset, data, size);
	// Update offset
	*request_offset += size;
}

ssize_t write_pipe(int fd, void *buffer, size_t buffer_size) {

	ssize_t written = 0;
	// Keeps trying to write if the pipe is full
	while (written < buffer_size) {
    	ssize_t ret = write(fd, buffer, buffer_size);

		ALWAYS_ASSERT(ret > 0, "write to pipe failed");
		written += ret;
	}

	return written;
}

ssize_t read_pipe(int fd, void *buffer, size_t buffer_size) {

    ssize_t ret = read(fd, buffer, buffer_size);
	ALWAYS_ASSERT(ret >= 0, "read from pipe failed");

	return ret;
}

ssize_t send_request(int fd, void* buffer, size_t buffer_size) {
	ssize_t ret;
	ret = write_pipe(fd, buffer, buffer_size);
	
	//free(buffer);

	return ret;
}

void *myalloc(size_t size) {
	void *ptr = malloc(size);
	ALWAYS_ASSERT(ptr, "No memory.")
	return ptr;
}