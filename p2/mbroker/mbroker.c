#include "logging.h"
#include "mbroker.h"
#include "betterassert.h"
#include "producer-consumer.h"
#include "pipeutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <pthread.h>

box *server_boxes;
pc_queue_t *queue;

int listen_for_requests(char* pipe_name) {
	int dummy_pipe;
	int server_pipe;
	char client_pipe[CLIENT_PIPE_LENGTH];
	uint8_t code;
	session ses;
	if ((server_pipe = open(pipe_name, O_RDONLY)) == -1) {
		return -1;
	}
	if ((dummy_pipe = open(pipe_name, O_WRONLY)) == -1) {
			return -1;
	}
	while(true) {
		read_pipe(server_pipe, &ses.code, sizeof(uint8_t));
		read_pipe(server_pipe, ses.pipe_name,
				sizeof(char) * CLIENT_PIPE_LENGTH);

		switch(ses.code) {
			case 7:
				break;

			case 1:
			case 2:
			case 3:
			case 5:
				read_pipe(server_pipe, ses.box_name,
						sizeof(char) * MAX_BOX_NAME);
				break;
			default:
				// Invalid code.
				return -1;
		}

		// Signal to workers.
		/* pcq_enqueue(queue, session); */
	}
	return 0;
} 



int main(int argc, char **argv) {
	/* int i; */
    if(argc == 2) {
		return -1;
	}
	int max_sessions = atoi(argv[2]);
	ALWAYS_ASSERT(max_sessions > 0, "Invalid session number\n");
	/* pthread_t worker_threads[max_sessions]; */
	unlink(argv[1]);
	
	if(mkfifo(argv[1], 0777) == -1){
		return -1;
	}

	if(listen_for_requests(argv[1]) == -1 ){
		return -1;
	}

	return 0;
	/*
	boxes = (box*) myalloc(sizeof(box) * max_boxes);
	queue = (pc_queue_t*) myalloc(sizeof(pc_queue_t));
	pcq_create(queue);

	for(i = 0; i < argv[2]; i++) {
		pthread_create(&worker_threads[i], NULL, &process_sessions, NULL);
	} 

	listen_for_requests(argv[1]);
	return 0;
	*/
} 

/*
void process_sessions() {
	// Waits for queue to not be empty
	session current = pcq_dequeue(queue);

	switch (current.code) {
		// Pick handler function for each type of session
	}
}
*/

