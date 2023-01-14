#ifndef MBROKER_H
#define MBROKER_H

#include "data.h"
#include "operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


tfs_params tfs_our_params () {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 20000,
    };
    return params;
}

void listen_for_requests(char* pipe_name);
void* process_sessions();

int handle_register_publisher(session_t *current);
uint8_t *build_subscriber_response(char *message);

int handle_register_subscriber(session_t *current);
uint8_t *build_subscriber_response(char *message);

uint8_t *build_create_box_response(int32_t ret_code, char *error_msg);
int handle_create_box(session_t *current);

#endif