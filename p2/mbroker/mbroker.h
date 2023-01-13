#ifndef MBROKER_H
#define MBROKER_H

#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void listen_for_requests(char* pipe_name);
int handle_register_publisher(session_t *current);

#endif