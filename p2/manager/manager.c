#include "logging.h"
#include "pipeutils.h"
#include "betterassert.h"
#include "manager.h"
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

uint8_t* build_manager_request(
		uint8_t code, char *pipe_name, char *box_name) {

	size_t request_size = REQUEST_WBOX_SIZE;

	uint8_t* request = (uint8_t*) myalloc(request_size);
	memset(request, 0, request_size);
	size_t request_offset = 0;

    requestcpy(request, &request_offset, &code, sizeof(uint8_t));
    requestcpy(request, &request_offset,
			pipe_name, CLIENT_PIPE_LENGTH * sizeof(char));
    requestcpy(request, &request_offset,
			box_name, BOX_NAME_LENGTH * sizeof(char));
	return request;
}

uint8_t* build_list_request(char *pipe_name) {
	size_t request_size = REQUEST_NO_BOX_SIZE;

	uint8_t* request = (uint8_t*) myalloc(request_size);
	memset(request, 0, request_size);
	size_t request_offset = 0;
	uint8_t code = MANAGER_LIST_CODE;

    requestcpy(request, &request_offset, &code, sizeof(uint8_t));
    requestcpy(request, &request_offset,
			pipe_name, CLIENT_PIPE_LENGTH * sizeof(char));
	return request;
}

int handle_create_remove(int rp_fd,
		char* pipe_name, char* box_name, uint8_t code) {

	int cp_fd;
	int32_t return_code;
	uint8_t response_code;
	char error_message[ERROR_MSG_LEN];

	send_request(rp_fd, build_manager_request(
			code, pipe_name, box_name), REQUEST_WBOX_SIZE);

	printf("este é o nome do pipe %s\n",pipe_name);
	ALWAYS_ASSERT(cp_fd = open(
			pipe_name, O_RDONLY) != -1, "Could not open client pipe.");
	printf("eu abri\n");

	read_pipe(cp_fd, &response_code, sizeof(uint8_t));
	printf("eu li code %u\n",response_code);
	read_pipe(cp_fd, &return_code, sizeof(uint32_t));
	printf("eu li return code %u\n",return_code);
	read_pipe(cp_fd, error_message, ERROR_MSG_LEN * sizeof(char));

	if (code == MANAGER_CREATE_CODE) {
		ALWAYS_ASSERT(response_code ==
				MANAGER_CREATE_RESPONSE_CODE,
				"Unexpected code read from pipe.");
	} else {
		ALWAYS_ASSERT(response_code ==
				MANAGER_REMOVE_RESPONSE_CODE,
				"Unexpected code read from pipe.");

	}
	if (return_code == 0) {
		fprintf(stdout, "OK\n");
	} else if (strlen(error_message) != 0) {
		fprintf(stdout, "ERROR %s\n", error_message);
	}
	return 0;
}

int parse_node(int cp_fd, box_node_t *node) {
	uint8_t code = 0;
	read_pipe(cp_fd, &code, sizeof(uint8_t));
	ALWAYS_ASSERT(code ==
			MANAGER_LIST_RESPONSE_CODE, "Unexpected code read from pipe.");

	read_pipe(cp_fd, &node->last, sizeof(uint8_t));
	read_pipe(cp_fd, &node->box_name, sizeof(MAX_BOX_NAME));
	if (node->last == 1 && strlen(node->box_name) == 0) {
		// No boxes
		return 1;
	}
	read_pipe(cp_fd, &node->box_size, sizeof(uint64_t));
	read_pipe(cp_fd, &node->n_publishers, sizeof(uint64_t));
	read_pipe(cp_fd, &node->n_subscribers, sizeof(uint64_t));
	node->next = NULL;
	return 0;
}

void print_boxes(box_node_t* head) {
	box_node_t* current = head;
	box_node_t* previous;
	while (current) {
		fprintf(stdout, "%s %zu %zu %zu\n",
				current->box_name, current->box_size,
				current->n_publishers,current->n_subscribers);
		previous = current;
		current = current->next;
        free(previous);
    }
}

box_node_t* process_list_response(int cp_fd) {
	box_node_t* head = (box_node_t*) myalloc(sizeof(box_node_t));
	box_node_t* new_node = head;
	box_node_t* current;
	if (parse_node(cp_fd, head) == 1) {
		free(head);
		fprintf(stdout, "NO BOXES FOUND\n");
		return NULL;
	}
	while (new_node->last == 0) {
		new_node = (box_node_t*) myalloc(sizeof(box_node_t));
		parse_node(cp_fd, new_node);
		// Compare to head of list
		if (strcmp(head->box_name, new_node->box_name) >= 0) {
			new_node->next = head;
			head = new_node;
		}
		current = head;
		while (current->next && strcmp(
				new_node->box_name, current->box_name) > 0) {

			current = current->next;
		}
		new_node->next = current->next;
		current->next = new_node;
	}
	return head;
}

int handle_list(int rp_fd, char* pipe_name) {

	int cp_fd;
	send_request(rp_fd, build_list_request(pipe_name), REQUEST_NO_BOX_SIZE);

	ALWAYS_ASSERT(cp_fd = open(
			pipe_name, O_RDONLY) != -1, "Could not open client pipe.");

	print_boxes(process_list_response(cp_fd));
	close(cp_fd);
	return 0;
}

int main(int argc, char **argv) {
    print_usage();
	int rp_fd;

	ALWAYS_ASSERT(argc == 5 || argc == 4, "Invalid arguments.");

	ALWAYS_ASSERT(mkfifo(argv[2], 0777) != -1, "Could not create pipe.");

	ALWAYS_ASSERT((rp_fd = open(
			argv[1], O_WRONLY)) != -1, "Could not open server pipe");
	
	char* operation = argv[3];

	if (strcmp(operation, MANAGER_BOX_CREATE) == 0) {
		return handle_create_remove(
				rp_fd, argv[2], argv[4], MANAGER_CREATE_CODE);
	}
	if (strcmp(operation, MANAGER_BOX_REMOVE) == 0) {
		return handle_create_remove(
				rp_fd, argv[2], argv[4], MANAGER_REMOVE_CODE);
	}
	if (strcmp(operation, MANAGER_BOX_LIST) == 0) {
		return handle_list(rp_fd, argv[2]);
	}

	PANIC("Invalid operation for manager.");
}
