#include "operations.h"
#include "logging.h"
#include "mbroker.h"
#include "betterassert.h"
#include "producer-consumer.h"
#include "pipeutils.h"
#include "locks.h"
#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <pthread.h>


void listen_for_requests(char* pipe_name) {
	int dummy_pipe;
	int server_pipe;
	session_t *ses;

	if ((server_pipe = open(pipe_name, O_RDONLY)) == -1) {
		PANIC("Cannot open register pipe.")
	}

	// Dummy pipe makes sure that there will always be at least
	// one write-end for the register pipe.
	if ((dummy_pipe = open(pipe_name, O_WRONLY)) == -1) {
		PANIC("Cannot open dummy pipe - aborting.")
	}

	while(true) {
		ses = (session_t*) myalloc(sizeof(session_t));
		read_pipe(server_pipe, &ses->code, sizeof(uint8_t));
		read_pipe(server_pipe, ses->pipe_name,sizeof(char) * CLIENT_PIPE_LENGTH);
		switch(ses->code) {
			case 7:
				// List boxes request (doesn't have box_name parameter)
				break;

			case 1:
			case 2:
			case 3:
			case 5:
				read_pipe(server_pipe, ses->box_name,
						sizeof(char) * MAX_BOX_NAME);
				break;

			default:
				// Invalid code.
				PANIC("Invalid code read by server.")
		}

		// Signal to workers.
		/* pcq_enqueue(queue, &session); */
	}
} 

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "Invalid arguments.");
	int max_sessions = atoi(argv[2]);
	ALWAYS_ASSERT(max_sessions > 0, "Invalid session number\n");
	/* pthread_t worker_threads[max_sessions]; */
	unlink(argv[1]);
	
	ALWAYS_ASSERT(mkfifo(argv[1], 0777) != -1, "Cannot create register pipe");

	ALWAYS_ASSERT(!tfs_init(NULL), "Cannot initialize tfs.");

	data_init();

	server_boxes = (box_t**) myalloc(sizeof(box_t**) * DEFAULT_BOX_LIMIT);
	queue = (pc_queue_t*) myalloc(sizeof(pc_queue_t));
	pcq_create(queue, DEFAULT_QUEUE_LENGTH);
	printf("Passei do pcq_create\n");

	/* for(i = 0; i < argv[2]; i++) {
		pthread_create(&worker_threads[i], NULL, &process_sessions, NULL);
	}  */

	listen_for_requests(argv[1]);
	return 0;

} 

int handle_register_publisher(session_t *current) {
	box_t *box;
	int cp_fd;
	int tfs_fd; 
	printf("HEYYY\n");
	if ((cp_fd = open(current->pipe_name, O_RDONLY)) == -1) {
		return -1;
	}
	if ((box = fetch_box(current->box_name)) == 0) {
		close(cp_fd);
		return -1;
	}

	if ((tfs_fd = tfs_open(box->path, TFS_O_APPEND)) == -1) {
		close(cp_fd);
		return -1;
	}
	mutex_lock(&box->content_mutex);
	if (!box || box->n_publishers > 0) {
		close(cp_fd);
		tfs_close(tfs_fd);
		mutex_unlock(&box->content_mutex);
		return -1;
	}/*
	if (box->status == CLOSED) {
		close(cp_fd);
		tfs_close(tfs_fd);
		mutex_unlock(&box->content_mutex);
		return -1;
	}
	box->pub_pipe_name = current->pipe_name;
	*/
	box->n_publishers++;
	mutex_unlock(&box->content_mutex);

	while (true) {
		uint8_t code;
		ssize_t ret;
		char message[MAX_MSG_LENGTH];
		ret = read_pipe(cp_fd, &code, sizeof(u_int8_t));
		mutex_lock(&box->content_mutex);
		/*if (box->status == CLOSED) {
			// Signals manager worker thread that is waiting for
			// publishers and subscribers to terminate their sessions.
			cond_signal(&box->condvar);
			mutex_unlock(&box->content_mutex);
			break;
		}*/

		if (ret == 0) {
			// Pipe closed
			break;
		}
		if (code != PUBLISH_CODE) {
			// Client did not respect wire protocol
			break;
		}

		ret = read_pipe(cp_fd, message, sizeof(char) * MAX_MSG_LENGTH);
		if (ret == 0) {
			// Pipe closed
			break;
		}
		printf("MESSAGE_RECEIVED[%s]\n",message);
		ret = tfs_write(tfs_fd, message, MAX_MSG_LENGTH);
		ALWAYS_ASSERT(ret != -1, "tfs_write failed");
		if (ret < MAX_MSG_LENGTH) {
			INFO("Box has ran out of space, no further messages will be sent!");
		}
		// Signal subs
		cond_broadcast(&box->condvar);
	}
	close(cp_fd);
	/* tfs_close(tfs_fd); */
	/* decrease_box_pubs(box); */
	return 0;
}

void process_sessions() {
	while (true) {
		// If queue is empty, waits for a producer signal.
		session_t *current = (session_t*) pcq_dequeue(queue);

		// Pick handler function for each type of session
		switch (current->code) {
			case 1:
				/* handle_register_publisher(current); */
				break;
			default:
				PANIC("Invalid code reached worker thread.");
		}
		free(current);
	}
}