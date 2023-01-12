#include "betterassert.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

void requestcpy(void *request,
		size_t *request_offset, void *data, size_t size) {

	memcpy(request + *request_offset, data, size);
	// Update offset
	*request_offset += size;
}

ssize_t write_pipe(int fd, void const *buffer) {
    size_t len = sizeof(buffer);
    ssize_t written = 0;

    while (written < len) {
        ssize_t ret = write(fd, buffer + written, len - (size_t)written);
		ALWAYS_ASSERT(ret > 0, "write to pipe failed");

        written += ret;
    }
	return written;
}

ssize_t read_pipe(int fd, void *buffer) {
    size_t len = sizeof(buffer);
    ssize_t ret = read(fd, buffer, len - 1);
	ALWAYS_ASSERT(ret >= 0, "read from pipe failed");

	return ret;
}

ssize_t send_request(int fd, char* buffer) {
	ssize_t ret;
	ret = write_pipe(fd, buffer);
	free(buffer);

	return ret;
}

void *myalloc(size_t size) {
	void *ptr = malloc(size);
	ALWAYS_ASSERT(ptr, "No memory.")
	return ptr;
}