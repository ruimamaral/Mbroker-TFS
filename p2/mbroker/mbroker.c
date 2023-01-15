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
		printf("code read by server->%u\n", ses->code);
		read_pipe(server_pipe,
				ses->pipe_name,sizeof(char) * CLIENT_PIPE_LENGTH);

		// Makes sure that the \0 character is always present
		ses->pipe_name[CLIENT_PIPE_LENGTH - 1] = '\0';
		switch(ses->code) {
			case MANAGER_LIST_CODE:
				// List boxes request (doesn't have box_name parameter)
				break;

			case PUB_CREATION_CODE:
			case SUB_CREATION_CODE:
			case MANAGER_CREATE_CODE:
			case MANAGER_REMOVE_CODE:
				read_pipe(server_pipe, ses->box_name,
						sizeof(char) * MAX_BOX_NAME);
				// Makes sure that the \0 character is always present
				ses->box_name[MAX_BOX_NAME - 1] = '\0';
				printf("Recebi pub/sub/create/remove code\n");
				break;

			default:
				// Invalid code.
				PANIC("Invalid code read by server.")
		}
		printf("before enqueue in listen_requests code->%u|pathname->%s|box_name->%s\n",ses->code,ses->pipe_name,ses->box_name);
		// Signal to workers.
		pcq_enqueue(queue, ses);
		
		printf("after enque in listen_requestscode-> %u ||| pipe_name-> %s ||| box_name-> %s\n",((session_t*)(queue->pcq_buffer[0]))->code,((session_t*)(queue->pcq_buffer[0]))->pipe_name,((session_t*)(queue->pcq_buffer[0]))->box_name);
	}
} 

int main(int argc, char **argv) {
    ALWAYS_ASSERT(argc == 3, "Invalid arguments.");
	int max_sessions = atoi(argv[2]);
	ALWAYS_ASSERT(max_sessions > 0, "Invalid session number\n");
	pthread_t worker_threads[max_sessions];
	unlink(argv[1]);
	
	ALWAYS_ASSERT(mkfifo(argv[1], 0777) != -1, "Cannot create register pipe");
	tfs_params params = tfs_our_params();

	ALWAYS_ASSERT(!tfs_init(&params), "Cannot initialize tfs.");

	data_init();

	for(int i = 0; i < max_sessions; i++) {
		pthread_create(&worker_threads[i], NULL, &process_sessions, NULL);
	}

	listen_for_requests(argv[1]);
	return 0;
} 

int handle_register_subscriber(session_t *current) {
	box_t *box;
	int cp_fd;
	int tfs_fd; 
	if ((cp_fd = open(current->pipe_name, O_WRONLY)) == -1) {
		return -1;
	}

	if ((box = box_add_sub(current->box_name)) == 0) {
		close(cp_fd);
		return -1;
	}

	if ((tfs_fd = tfs_open(box->path, 0)) == -1) {
		close(cp_fd);
		mutex_lock(&box->content_mutex);
		box->n_subscribers--;
		mutex_unlock(&box->content_mutex);
		return -1;
	}
	
	while (true) {
		ssize_t ret;
		char message[MAX_MSG_LENGTH];

		// Lock to prevent signal leakage and/or data races from accessing box.
		mutex_lock(&box->content_mutex);

		while ((ret = tfs_read(tfs_fd, message,
				MAX_MSG_LENGTH)) == 0 && box->status == NORMAL) {

			cond_wait(&box->condvar, &box->content_mutex);
		}
		// If the box is under the process of removal by a manager thread,
		// we end our session by decreasing subscriber amount and leaving.
		if (box->status == CLOSED) {
			close(cp_fd);
			tfs_close(tfs_fd);
			box->n_subscribers--;
			// Last subscriber will signal the manager worker thread,
			// which will see that no subscribers are left and it will
			// then safely close the box.
			cond_signal(&box->condvar);
			mutex_unlock(&box->content_mutex);
			return 0;
		}
		mutex_unlock(&box->content_mutex);

		if (ret < 0) {
			// tfs_read failed
			break;
		}
		send_request(cp_fd,
				build_subscriber_response(message), SUBSCRIBER_RESPONSE_SIZE);

	}
	close(cp_fd);
	tfs_close(tfs_fd);

	mutex_lock(&box->content_mutex);
	box->n_subscribers--;
	// Broadcast in order to prevent infinite waiting by a manager thread
	// trying to remove the box.
	cond_broadcast(&box->condvar);
	mutex_unlock(&box->content_mutex);

	return 0;
}

uint8_t *build_subscriber_response(char *message) {
	uint8_t code = SUBSCRIBER_RESPONSE_CODE;
	size_t size = SUBSCRIBER_RESPONSE_SIZE;
	uint8_t* response = (uint8_t*) myalloc(size);
	memset(response, 0, size);
	size_t offset = 0;

    requestcpy(response, &offset, &code, sizeof(uint8_t));
    requestcpy(response, &offset,
		message, MAX_MSG_LENGTH * sizeof(char));
	return response;
}

int handle_register_publisher(session_t *current) {
	box_t *box;
	int cp_fd;
	int tfs_fd; 
	printf("cheguei\n");
	if ((cp_fd = open(current->pipe_name, O_RDONLY)) == -1) {
		return -1;
	}
	printf("ok1\n");
	if ((box = box_add_pub(current->box_name)) == 0) {
		// Box doesn't exist or it already has a publisher
		close(cp_fd);
		return -1;
	}
	printf("ok2\n");
	if ((tfs_fd = tfs_open(box->path, TFS_O_APPEND)) == -1) {
		close(cp_fd);
		mutex_lock(&box->content_mutex);
		box->n_publishers--;
		mutex_unlock(&box->content_mutex);
		return -1;
	}
	// Gives box the publisher pipename so that we can unlock it from
	// a blocking read during the box removal process.
	memcpy(box->pub_pipe_name,
			current->pipe_name, CLIENT_PIPE_LENGTH * sizeof(char));

	printf("ok3\n");
	while (true) {
		uint8_t code;
		ssize_t ret;
		char message[MAX_MSG_LENGTH];

		ret = read_pipe(cp_fd, &code, sizeof(uint8_t));
		printf("handle_publisher read_code->%zu",ret);
		printf("handle_publisher readcode->%u\n",code);
		// Lock to prevent signal leakage and/or data races from accessing box.
		mutex_lock(&box->content_mutex);
		// Checks if the box is undergoing the process of removal.
		if (box->status == CLOSED) {
			close(cp_fd);
			tfs_close(tfs_fd);
			box->n_publishers--;
			// Signals manager worker thread that is waiting for
			// publishers and subscribers to terminate their sessions.
			cond_signal(&box->condvar);
			mutex_unlock(&box->content_mutex);
			return 0;
		}
		mutex_unlock(&box->content_mutex);

		if (ret == 0) {
			// Pipe closed
			break;
		}
		if (code != PUBLISH_CODE) {
			// Client did not respect wire protocol
			break;
		}

		ret = read_pipe(cp_fd, message, sizeof(char) * MAX_MSG_LENGTH);
		printf("handle_register_publisher read->%zu\n",ret);
		if (ret == 0) {
			// Pipe closed
			break;
		}
		printf("MESSAGE_RECEIVED[%s]\n",message);
		ret = tfs_write(tfs_fd, message, MAX_MSG_LENGTH);
		if (ret < MAX_MSG_LENGTH) {
			break; // Box ran out of space or write failed
		}
		mutex_lock(&box->content_mutex);
		box->box_size += (uint64_t) ret;
		// Signal subs
		cond_broadcast(&box->condvar);
		mutex_unlock(&box->content_mutex);
	}
	close(cp_fd);
	tfs_close(tfs_fd);

	mutex_lock(&box->content_mutex);
	box->n_publishers--;
	// Broadcast in order to prevent infinite waiting by a manager thread
	// trying to remove the box.
	cond_broadcast(&box->condvar);
	mutex_unlock(&box->content_mutex);

	return 0;
}

uint8_t *build_manager_response(
		uint8_t code, int32_t ret_code, char *error_msg) {
	
	printf("building man response! > %d\n", code);

	size_t size = MANAGER_RESPONSE_SIZE;
	uint8_t* response = (uint8_t*) myalloc(size);
	memset(response, 0, size);
	size_t offset = 0;

    requestcpy(response, &offset, &code, sizeof(uint8_t));
	requestcpy(response, &offset, &ret_code, sizeof(int32_t));
    requestcpy(response, &offset, error_msg, ERROR_MSG_LEN * sizeof(char));
	return response;
}

int handle_create_box(session_t *current) {
	int ret;
	int32_t ret_code = 0;
	int cp_fd;
	uint8_t response_code = MANAGER_CREATE_RESPONSE_CODE;
	char *error_msg = (char*) myalloc(ERROR_MSG_LEN * sizeof(char));
	memset(error_msg, 0, ERROR_MSG_LEN * sizeof(char));
	printf("este será o pipe do create %s\n",current->pipe_name);
	if ((cp_fd = open(current->pipe_name, O_WRONLY)) == -1) {
		return -1;
	}
	printf("file descriptor IS: %d", cp_fd);

	ret = box_create(current->box_name);

	switch (ret) {
		case 0:
			break;
		case -1:
			SET_ERROR(error_msg, ERR_BOX_EXISTS, ret_code);
			break;
		case -2:
			SET_ERROR(error_msg, ERR_TOO_MANY_BOXES, ret_code);
			break;
		case -3:
			SET_ERROR(error_msg, ERR_TFS_FAIL, ret_code);
			break;
		default:
			return -1;
	}
	printf("handle_creat_box sending request: %d, %s\n", ret_code, error_msg);
	send_request(cp_fd,
			build_manager_response(response_code,
			ret_code, error_msg), MANAGER_RESPONSE_SIZE);
	printf("voltei\n");
	printf("handle_creat_box sent request: %d, %s\n", ret_code, error_msg);

	free(error_msg);

	sleep(1); // Gives client time to read the server response
	close(cp_fd);
	printf("closed the damn pipe my guy\n");
	return ret;
} 

int handle_remove_box(session_t* current) {
	int ret;
	int32_t ret_code = 0;
	int cp_fd;
	void *error_msg = (void*) myalloc(ERROR_MSG_LEN * sizeof(char));
	memset(error_msg, 0, ERROR_MSG_LEN * sizeof(char));

	if ((cp_fd = open(current->pipe_name, O_WRONLY)) == -1) {
		return -1;
	}

	ret = box_remove(current->box_name);

	switch (ret) {
		case 0:
			break;
		case -1:
			SET_ERROR(error_msg, ERR_BOX_DOESNT_EXIST, ret_code);
			break;
		default:
			return -1;
	}
	send_request(cp_fd,
			build_manager_response(MANAGER_REMOVE_RESPONSE_CODE,
			ret_code, error_msg), MANAGER_RESPONSE_SIZE);

	free(error_msg);

	sleep(1); // Gives client process time to read the server response
	close(cp_fd);
	return ret;
}

uint8_t *build_list_boxes_response(uint8_t last, box_t* box) {
	uint8_t code = MANAGER_LIST_RESPONSE_CODE;
	size_t size = MANAGER_LIST_RESPONSE_SIZE;
	uint8_t* response = (uint8_t*) myalloc(size);
	memset(response, 0, size);
	size_t offset = 0;

    requestcpy(response, &offset, &code, sizeof(uint8_t));
	requestcpy(response, &offset, &last, sizeof(int8_t));
	requestcpy(response, &offset, box->name, MAX_BOX_NAME * sizeof(char));
    requestcpy(response, &offset, &box->box_size, sizeof(uint64_t));
    requestcpy(response, &offset, &box->n_publishers, sizeof(uint64_t));
    requestcpy(response, &offset, &box->n_subscribers, sizeof(uint64_t));
	return response;
}

int handle_list_boxes(session_t* current) {
	uint8_t last = 0;
	size_t box_amount;
	box_t **boxes;
	int cp_fd;

	if ((cp_fd = open(current->pipe_name, O_WRONLY)) == -1) {
		return -1;
	}

	boxes = box_get_all(&box_amount);

	if (box_amount == 0) {
		last = 1;
		char *box_name = (char*) myalloc(sizeof(char) * MAX_BOX_NAME);
		size_t response_size = 
				2 * sizeof(uint8_t) + sizeof(char) * MAX_BOX_NAME;
		uint8_t *response = (uint8_t*) myalloc(response_size);
		uint8_t code = MANAGER_LIST_RESPONSE_CODE;
		size_t offset = 0;
		memset(box_name, 0, MAX_BOX_NAME);
		memset(response, 0, response_size);
		requestcpy(response, &offset, &code, sizeof(uint8_t));
		requestcpy(response, &offset, &last, sizeof(uint8_t));
		requestcpy(response, &offset,
				box_name, sizeof(MAX_BOX_NAME * sizeof(char)));
		send_request(cp_fd, response, response_size);
		free(box_name);
		return 0;
	}
	for (int i = 0; i < box_amount; i++) {
		if (i == box_amount - 1) {
			last = 1;
		}
		send_request(cp_fd, build_list_boxes_response(
				last, boxes[i]), MANAGER_LIST_RESPONSE_SIZE);
		free(boxes[i]);
	}
	free(boxes);
	return 0;
}

void* process_sessions() {
	while (true) {
		// If queue is empty, waits for a producer signal.
		printf("waiting for next request\n");
		session_t *current = (session_t*) pcq_dequeue(queue);
		printf("process_sessions->saiu da queue\n");
		printf("process sessions || code->%u|pathname->%s|box_name->%s\n",current->code,current->pipe_name,current->box_name);
		// Pick handler function for each type of session
		switch (current->code) {
			case PUB_CREATION_CODE:
				printf("huh\n");
				handle_register_publisher(current);
				break;
			case SUB_CREATION_CODE:
				handle_register_subscriber(current);
				break;
			case MANAGER_CREATE_CODE:
				handle_create_box(current);
				break;
			case MANAGER_REMOVE_CODE:
				handle_remove_box(current);
				break;
			case MANAGER_LIST_CODE:
				handle_list_boxes(current);
				break;
			default:
				PANIC("Invalid code reached worker thread.");
		}
		free(current);
	}
}