#include "logging.h"
#include "pipeutils.h"
#include "betterassert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

static void print_usage() {
    fprintf(stderr, "usage: \n"
            "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
            "   manager <register_pipe_name> <pipe_name> list\n");
}

uint8_t *build_create_box_request(char *pipe_name, char *box_name) {
	size_t request_len = sizeof(uint8_t)
			+ (BOX_NAME_LENGTH + CLIENT_PIPE_LENGTH) * sizeof(char);

	void* request = (void*) myalloc(request_len);
	memset(request, 0, request_len);
	uint8_t code = 3; // TEMP fix this
	size_t request_offset = 0;

    requestcpy(request, &request_offset, &code, sizeof(uint8_t));
    requestcpy(request, &request_offset,
			pipe_name, CLIENT_PIPE_LENGTH * sizeof(char));
    requestcpy(request, &request_offset,
			box_name, BOX_NAME_LENGTH * sizeof(char));
	return request;
}

int main(int argc, char **argv) {
    print_usage();
	char *pipe_name = argv[2];
	char *response = (char*) myalloc(MANAGER_RESPONSE_SIZE);
	int rp_fd, cp_fd;
	ALWAYS_ASSERT(argc == 5, "Invalid arguments.");
	ALWAYS_ASSERT(mkfifo(pipe_name, 0777) != -1, "Could not create pipe.");
	ALWAYS_ASSERT((rp_fd = open(argv[1],
			O_WRONLY)) != -1, "Could not open server pipe");
	
	// O tamanho é igual ao do publish... depois muda-se
	send_request(rp_fd, build_create_box_request(
			pipe_name, argv[4]), REQUEST_PUBLISH_LEN);
	ALWAYS_ASSERT(cp_fd = open(
			pipe_name, O_RDONLY) != -1, "Could not open client pipe.");

	read_pipe(cp_fd, response, MANAGER_RESPONSE_SIZE);
	
    return -1;
}
