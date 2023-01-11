#include "logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <pthread.h>
#include "betterassert.h"

int main(int argc, char **argv) {
	int i;
    if( argc == 2 ) {
		return -1;
	}
	ALWAYS_ASSERT((int)argv[2] > 0,"Invalid session number\n");
	pthread_t worker_threads[(int)argv[2]];
	unlink(argv[1]);

	if( mkfifo( argv[1],0777 ) == -1 ){
		return -1;
	}

	/* for( i = 0; i < argv[2] ; i++) {
		pthread_create(&worker_threads[0],NULL,&process_session,NULL);
	} */

	init_queue();
	accept_register(argv[1]);
}


accept_register(char* pipe_name) {
	int dummy_pipe;
	int server_pipe;
	ssize_t rest_of_message= 0;
	if((server_pipe = open(pipe_name, O_RDONLY)) == -1 ){
		return -1;
	}
	if((dummy_pipe = open(pipe_name, O_WRONLY)) == -1 ){
			return -1;
	}
	while(true) {
		ssize_t ret = read(server_pipe, , sizeof(u_int8_t));
		switch(code) {
			case:>"1" 

		}
	}
}