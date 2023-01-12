#include "logging.h"
#include "pipeutils.h"
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



char *publish_request(char *message) {
	size_t request_len = sizeof(uint8_t) + MAX_MSG_LENGTH * sizeof(char);

	char* request = (char*) myalloc(request_len);
	memset(request, 0, REQUEST_PUBLISH_LEN);
	uint8_t code = PUBLISH_CODE;
	size_t request_offset = 0;

    requestcpy(request, &request_offset, &code, sizeof(uint8_t));
    requestcpy(request, &request_offset,
			message, MAX_MSG_LENGTH * sizeof(char));
	return request;

}

void process_messages(int fd) {
	char buffer[MAX_MSG_LENGTH];
	int len = 0;
	char c;
	memset(buffer, 0, MAX_MSG_LENGTH);

	while (true) {
		while ((c = (char)getchar()) != '\n') {
			if (c == EOF) {
				return;
			}
			if (len >= MAX_MSG_LENGTH - 1) {
				// Truncate text
				while ((c = (char)getchar()) != '\n') {
					if (c == EOF) {
						return;
					}
				}
				break;
			}
			buffer[len++] = c;
		}
		memset(buffer, 0, MAX_MSG_LENGTH);
		len = 0;
		if (send_request(fd, publish_request(buffer),MAX_MSG_LENGTH) <= 0) { // TEMP check if needed
			return;
		}
	}
}



void *creation_request(char *pipe_name, char *box_name) {
	printf("entrei\n");
	size_t request_len = sizeof(uint8_t)
			+ (BOX_NAME_LENGTH + CLIENT_PIPE_LENGTH) * sizeof(char);

	void* request = (void*) myalloc(request_len);
	memset(request, 0, request_len);
	uint8_t code = PUB_CREATION_CODE;
	size_t request_offset = 0;

    requestcpy(request, &request_offset, &code, sizeof(uint8_t));
    requestcpy(request, &request_offset,
			pipe_name, CLIENT_PIPE_LENGTH * sizeof(char));
    requestcpy(request, &request_offset,
			box_name, BOX_NAME_LENGTH * sizeof(char));
	return request;
}

int main(int argc, char **argv) {
	int rp_fd;
	int fd;
	char* pipe_name = argv[2];
    if (argc != 4) {
        printf("Number of input arguments is incorrect\n");
        return -1;
    }

	if (mkfifo(pipe_name, 0777) == -1) {
        printf("Unable to send create client pipe\n");
        return -1;
    }

    if ((rp_fd = open(argv[1], O_WRONLY)) == -1) {
		printf("Unable to open server pipe\n");
        return -1;
    }

    if (send_request(rp_fd, creation_request(pipe_name, argv[3]),REQUEST_PUBLISH_LEN) == -1 ) {
		close(rp_fd);
		unlink(pipe_name);
        printf("Unable to send request to server\n");
        return -1;
    }

	// Waits for pipe to be opened server-side
	printf("AYOOOO\n");
	if ((fd = open(pipe_name, O_WRONLY)) == -1) {
		close(rp_fd);
		unlink(pipe_name);
        printf("Cannot open client pipe\n");
		return -1;
	}
	printf("ola\n");
	process_messages(fd);

	destroy(pipe_name, fd, rp_fd);

	printf("Publisher terminated.");

    return 0;
}
