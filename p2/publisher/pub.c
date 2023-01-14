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


/// @brief closes all the the pipes and unlikss the client_pipe
/// @param pipe_name 
/// @param fd server pipe's file descriptor
/// @param rp_fd client pipe's file descriptor
void destroy(char *pipe_name, int fd, int rp_fd) {
	close(fd);
	close(rp_fd);
    unlink(pipe_name);
}


/// @brief creates message request
/// @param message 
/// @return the message request
void *publish_request(char *message) {
	printf("entrei no publish\n");
	size_t request_len = sizeof(uint8_t) + MAX_MSG_LENGTH * sizeof(char);

	void* request = (void*) myalloc(request_len);
	memset(request, 0, request_len);
	uint8_t code = PUBLISH_CODE;
	size_t request_offset = 0;

    requestcpy(request, &request_offset, &code, sizeof(uint8_t));
    requestcpy(request, &request_offset,
		message, MAX_MSG_LENGTH * sizeof(char));
	return request;

}


/// @brief Waits for input and sends a message request if an /n or EOF is inputted(closing the process with the latest)
/// @param fd client pipe's file descriptor
void process_messages(int fd) {
	char buffer[MAX_MSG_LENGTH];
	int len = 0;
	char c;
	memset(buffer, 0, MAX_MSG_LENGTH);

	while (true) {
		while ((c = (char)getchar()) != '\n') {
			if (c == EOF) {
				break;
			}
			if (len >= MAX_MSG_LENGTH - 1) {
				// Truncate text
				while ((c = (char) getchar()) != '\n') {
					if (c == EOF) {
						return;
					}
				}
				break;
			}
			buffer[len++] = c;
		}
		printf("what\n");
		len = 0;
		if (send_request(fd, publish_request(buffer),MAX_MSG_LENGTH) < 0) { // creates and writes the message request in the pipe
			return;
		}
		printf("MESSAGE_SENT[%s]\n",buffer);
		if (c == EOF) {
			break;
		}
		memset(buffer, 0, MAX_MSG_LENGTH);
	}
}


/// @brief 
/// @param pipe_name 
/// @param box_name 
/// @return the request shape [ code = 1 (uint8_t) | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
void *creation_request(char *pipe_name, char *box_name) {
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



/// @brief Receives information, and proceeds to send a Request with the shape
// 			[ code = 1 (uint8_t) | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
//			and if accepted sends messages with the shape [ code = 9 (uint8_t) ] | [ message (char[1024]) ]
/// @param argc 
/// @param argv 
/// @return
int main(int argc, char **argv) {
	int rp_fd;
	int fd;
	char* pipe_name = argv[2];
    if (argc != 4) {
        printf("Number of input arguments is incorrect\n");
        return -1;
    }

	unlink(pipe_name);

	if (mkfifo(pipe_name, 0777) == -1) {
        printf("Unable to send create client pipe\n");
        return -1;
    }

	//opens the server's pipe on write mode
    if ((rp_fd = open(argv[1], O_WRONLY)) == -1) {
		printf("Unable to open server pipe\n");
        return -1;
    }
	
	//creates a request and proceeds to write it in the server pipe
    if (send_request(rp_fd,
			creation_request(pipe_name, argv[3]), REQUEST_PUBLISH_LEN) == -1 ) {
		close(rp_fd);
		unlink(pipe_name);
        printf("Unable to send request to server\n");
        return -1;
    }

	// Waits for pipe to be opened server-side
	if ((fd = open(pipe_name, O_WRONLY)) == -1) {
		close(rp_fd);
		unlink(pipe_name);
        printf("Cannot open client pipe\n");
		return -1;
	}

	process_messages(fd);

	destroy(pipe_name, fd, rp_fd);

	printf("Publisher terminated.");

    return 0;
}
