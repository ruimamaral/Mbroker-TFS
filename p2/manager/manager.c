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

uint8_t* build_manager_request(uint8_t code,char *pipe_name, char *box_name) {
	size_t request_len = sizeof(uint8_t)
			+ (BOX_NAME_LENGTH + CLIENT_PIPE_LENGTH) * sizeof(char);

	void* request = (void*) myalloc(request_len);
	memset(request, 0, request_len);
	size_t request_offset = 0;

    requestcpy(request, &request_offset, &code, sizeof(uint8_t));
    requestcpy(request, &request_offset,
			pipe_name, CLIENT_PIPE_LENGTH * sizeof(char));
    requestcpy(request, &request_offset,
			box_name, BOX_NAME_LENGTH * sizeof(char));
	return request;
}

int read_return_request_function(int rp_fd,char* pipe_name,char* box_name,uint8_t code) {
	int cp_fd;
	int32_t return_code;
	char error_message[MAX_MSG_LENGTH];
	if(send_request(rp_fd, build_manager_request(code,pipe_name,box_name), REQUEST_PUBLISH_LEN)){
		return -1;
	}
	ALWAYS_ASSERT(cp_fd = open(pipe_name, O_RDONLY) != -1, "Could not open client pipe.");
	read_pipe(cp_fd,&code,sizeof(uint8_t));
	read_pipe(cp_fd,&return_code,sizeof(uint32_t));
	read_pipe(cp_fd,&error_message,MAX_MSG_LENGTH);

	if(code != MANAGER_CREATE_RESPONSE_CODE) {
		close(cp_fd);
		return -1;
	}
	ALWAYS_ASSERT(return_code == 0 || return_code == 1, "Return code invalid");
	if(return_code == 0) {
		fprintf(stdout,"OK\n");
	}
	else if( strlen(error_message) != 0) {
		fprintf(stdout,"ERROR %s\n",error_message);
	}
	return 0;
}

int  parse_node(int cp_fd,box_node* node, uint64_t* last) {
	read_pipe(cp_fd,last,sizeof(uint8_t));
	read_pipe(cp_fd,&node->box_name,sizeof(MAX_BOX_NAME));
	if( *last == 1) {
		if( strlen(node->box_name) == 0 ){
			return 1;
		}
	}
	read_pipe(cp_fd,&node->box_size,sizeof(uint64_t));
	read_pipe(cp_fd,&node->n_publishers,sizeof(uint64_t));
	read_pipe(cp_fd,&node->n_subscribers,sizeof(uint64_t));
	node->next = NULL;
	return 0;
}

void print_boxes(box_node* head) {
	box_node* current;
	while(head) {
		current = head;
		head= head->next;
		fprintf(stdout, "%s %zu %zu %zu\n", current->box_name, current->box_size, current->n_publishers,current->n_subscribers);
        free(current);
    }
}

int get_boxes(int cp_fd) {
	uint64_t last;
	box_node* head = (box_node*) myalloc(sizeof(box_node));
	if(parse_node(cp_fd,head,&last) == 1) {
		free(head);
		fprintf(stdout,"NO BOXES FOUND\n");
	}
	else{
		while (last == 0) {
			box_node* new_node = (box_node*) myalloc(sizeof(box_node));
			parse_node(cp_fd,new_node,&last);
			box_node* current = head;
			while (current->next && strcmp(new_node->box_name, current->box_name) < 0){
				current = current->next;
			}
			current = new_node->next;
		}
		print_boxes(head);
	}
	return 0;
}



int treat_list_request(int rp_fd,char* pipe_name,char* box_name,uint8_t code){
	int cp_fd;
	if(send_request(rp_fd, build_manager_request(code,pipe_name,box_name), REQUEST_PUBLISH_LEN)){
		return -1;
	}
	ALWAYS_ASSERT(cp_fd = open(pipe_name, O_RDONLY) != -1, "Could not open client pipe.");
	get_boxes(cp_fd);
	close(cp_fd);
	return 0;

}



int main(int argc, char **argv) {
    print_usage();
	char *pipe_name = argv[2];
	int rp_fd;
	int answer;

	ALWAYS_ASSERT(argc == 5 || argc == 4, "Invalid arguments.");
	ALWAYS_ASSERT(mkfifo(pipe_name, 0777) != -1, "Could not create pipe.");
	ALWAYS_ASSERT((rp_fd = open(argv[1],
			O_WRONLY)) != -1, "Could not open server pipe");
	
	char* manager_action = argv[3];

	if(strcmp(manager_action,MANAGER_BOX_CREATE) == 0) {
		answer = read_return_request_function(rp_fd,pipe_name,argv[4],MANAGER_CREATE_CODE);
	}
	else if(strcmp(manager_action,MANAGER_BOX_REMOVE) == 0) {
		answer = read_return_request_function(rp_fd,pipe_name,argv[4],MANAGER_REMOVE_CODE);
	}
	else if(strcmp(manager_action,MANAGER_BOX_LIST) == 0) {
		answer = treat_list_request(rp_fd,pipe_name, argv[4],MANAGER_LIST_CODE);
	}
	else{
		// Invalid code.
		PANIC("Invalid action for manager.");
	}
	// O tamanho é igual ao do publish... depois muda-se
	return answer;
}
