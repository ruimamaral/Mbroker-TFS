#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void mutex_init(pthread_mutex_t *mutex) {
	if (pthread_mutex_init(mutex, NULL) != 0) {
		exit(EXIT_FAILURE);
	}
}