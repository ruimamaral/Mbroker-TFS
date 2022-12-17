#ifndef LOCKS_H
#define LOCKS_H

#include <pthread.h>

void mutex_init(pthread_mutex_t *mutex);
void mutex_lock(pthread_mutex_t *mutex);
void mutex_unlock(pthread_mutex_t *mutex);
void mutex_kill(pthread_mutex_t *mutex);

#endif