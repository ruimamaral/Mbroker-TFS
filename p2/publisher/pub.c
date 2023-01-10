#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>



void requestcpy(void* request, size_t* request_offset, void* data, size_t size) {
    memcpy(request + *request_offset, data, size);
    *request_offset += size;
}

int send_request(char** argv) {
	int pd;
	u_int8_t code = 1;
	size_t request_offset = 0;
	char request[289];
	if((pd = open(argv[1], O_WRONLY)) == -1 ){
		return -1;
	}
	memset(request, 0, sizeof(request));
	requestcpy(request, &request_offset, &code, sizeof(u_int8_t));
	requestcpy(request, &request_offset, argv[1], sizeof(char)*256);
	requestcpy(request, &request_offset, argv[2], sizeof(char)*32);

	ssize_t ret = write(pd, request , sizeof(request));
	if (ret < 0) {
		return -1;
	}
	close(pd);
	return 0;
}

int send_msg(char* client_pipe) {
	int cp;
	int character;
	ssize_t ret;
	int i = 0;
	if((cp = open(client_pipe, O_WRONLY)) == -1 ){
		return -1;
	}

	char message[1024];
	while( ((character = getchar()) != EOF)) {
		if( character == '\n' ) {
			i = 0;
			if ((ret = write(cp,message, sizeof(message))) < 0) {
				return -1;
			}
			memset(message,0,sizeof(message));
		}
		else{
			message[i] = (char)character;
		}
		i++;
	}
	close(cp);
	return 0; 
}



int main(int argc, char **argv) {
    if(argc != 4) {
		printf("sai1\n");
		return -1;
	}

	unlink(argv[2]);

	if (mkfifo(argv[2],0777) == -1 ){
		printf("sai3\n");
		return -1;
	}

	if(send_request(argv) != 0 ) {
		printf("sai4\n");
		return -1;
	}
	if(send_msg(argv[2]) != 0 ){
		printf("sai5\n");
		return -1;
	}

    return 0;
}

