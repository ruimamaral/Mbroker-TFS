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
uint8_t* publish_request(char *message) {
	printf("entrei no publish\n");
	size_t request_len = sizeof(uint8_t) + MAX_MSG_LENGTH * sizeof(char);

	uint8_t* request = (uint8_t*) myalloc(request_len);
	memset(request, 0, request_len);
	uint8_t code = PUBLISH_CODE;
	size_t request_offset = 0;

    requestcpy(request, &request_offset, &code, sizeof(uint8_t));
    requestcpy(request, &request_offset,
		message, MAX_MSG_LENGTH * sizeof(char));
	return request;

}


/// @brief Waits for input and sends a message request if an /n	or EOF
///	is inputted (closing the process with the latest)
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
			// Reads until max length and truncates the rest of the message.
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
 		// creates and writes the message request in the pipe
		send_request(fd, publish_request(buffer),MAX_MSG_LENGTH);
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
/// @return the request format [ code = 1 (uint8_t) |
///	[ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
uint8_t* creation_request(char *pipe_name, char *box_name) {
	size_t request_len = sizeof(uint8_t)
			+ (BOX_NAME_LENGTH + CLIENT_PIPE_LENGTH) * sizeof(char);

	uint8_t* request = (uint8_t*) myalloc(request_len);
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



/// @brief Receives information, and proceeds to send a Request with the format
/// 		[ code = 1 (uint8_t) | [ client_named_pipe_path (char[256]) ]
///			| [ box_name (char[32]) ] and if accepted sends messages
///			with the format [ code = 9 (uint8_t) ] | [ message (char[1024]) ]
/// @param argc 
/// @param argv 
/// @return
int main(int argc, char **argv) {
	int rp_fd;
	int fd;
	char* pipe_name = argv[2];

    ALWAYS_ASSERT(argc == 4, "Invalid arguments.");

	ALWAYS_ASSERT(mkfifo(
			pipe_name, 0777) != -1, "Unable to create client pipe");

	//opens the server's pipe on write mode
	ALWAYS_ASSERT((rp_fd = open(
			argv[1], O_WRONLY)) != -1, "Unable to open server pipe");

	// creates a request and proceeds to write it in the server pipe
	send_request(rp_fd,
			creation_request(pipe_name, argv[3]), REQUEST_WBOX_SIZE);
	printf("MANDEI\n");

	// Waits for pipe to be opened server-side
	ALWAYS_ASSERT((fd = open(
			pipe_name, O_RDONLY)) != -1, "Cannot open pipe.");

	process_messages(fd);

	destroy(pipe_name, fd, rp_fd);

	printf("Publisher terminated.\n");

    return 0;
}
