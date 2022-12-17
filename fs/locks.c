#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void mutex_init(pthread_mutex_t *mutex) {
	if (pthread_mutex_init(mutex, NULL) != 0) {
		exit(EXIT_FAILURE);
	}
}
void mutex_lock(pthread_mutex_t *mutex) {
	if (pthread_mutex_lock(mutex) != 0) {
		exit(EXIT_FAILURE);
	}
}
void mutex_unlock(pthread_mutex_t *mutex) {
	if (pthread_mutex_unlock(mutex) != 0) {
		exit(EXIT_FAILURE);
	}
}
void mutex_kill(pthread_mutex_t *mutex) {
	if (pthread_mutex_destroy(mutex) != 0) {
		exit(EXIT_FAILURE);
	}
}