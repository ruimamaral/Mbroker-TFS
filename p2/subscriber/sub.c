#include "logging.h"
#include "pipeutils.h"
#include "betterassert.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>


void destroy(char *pipe_name, int fd, int rp_fd) {
	close(fd);
	close(rp_fd);
    unlink(pipe_name);
}

uint8_t* creation_request(char *pipe_name, char *box_name) {
	printf("entrei\n");
	size_t request_len = sizeof(uint8_t)
			+ (BOX_NAME_LENGTH + CLIENT_PIPE_LENGTH) * sizeof(char);

	uint8_t* request = (uint8_t*) myalloc(request_len);
	memset(request, 0, request_len);
	uint8_t code = SUB_CREATION_CODE;
	size_t request_offset = 0;

    requestcpy(request, &request_offset, &code, sizeof(uint8_t));
    requestcpy(request, &request_offset,
			pipe_name, CLIENT_PIPE_LENGTH * sizeof(char));
    requestcpy(request, &request_offset,
			box_name, BOX_NAME_LENGTH * sizeof(char));
	return request;
}

int subscribe(int fd) {
	uint8_t code;
	char buffer[MAX_MSG_LENGTH];
	while (true) {
		ssize_t ret = read_pipe(fd, &code, sizeof(uint8_t));
		if (code != SUB_RECEIVE_CODE) {
			return -1;
		}
		ret = read_pipe(fd, buffer, sizeof(buffer));
		if (ret <= 0) {
			break;
		}
		printf("%s\n", buffer);
		memset(buffer, 0, MAX_MSG_LENGTH);
	}
	return 0;
}


int main(int argc, char **argv) {
   	int rp_fd;
	int fd;
	char* pipe_name = argv[2];

    ALWAYS_ASSERT(argc == 4, "Invalid arguments.");

	ALWAYS_ASSERT(mkfifo(
			pipe_name, 0777) != -1, "Unable to create client pipe");

	ALWAYS_ASSERT((rp_fd = open(
			argv[1], O_WRONLY)) != -1, "Unable to open server pipe");

    send_request(rp_fd,
			creation_request(pipe_name, argv[3]), REQUEST_WBOX_SIZE);

	// Waits for pipe to be opened server-side
	ALWAYS_ASSERT((fd = open(
			pipe_name, O_RDONLY)) != -1, "Cannot open pipe.");

	ALWAYS_ASSERT(subscribe(fd) != -1, "Unexpected code read from pipe");

	destroy(pipe_name, fd, rp_fd);

	printf("Subscriber terminated.\n");

    return 0;
}