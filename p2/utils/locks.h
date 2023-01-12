#ifndef LOCKS_H
#define LOCKS_H

#include <pthread.h>

void mutex_init(pthread_mutex_t *mutex);
void mutex_lock(pthread_mutex_t *mutex);
void mutex_unlock(pthread_mutex_t *mutex);
void mutex_kill(pthread_mutex_t *mutex);

void rwlock_init(pthread_rwlock_t *rwlock);
void rwlock_rdlock(pthread_rwlock_t *rwlock);
void rwlock_wrlock(pthread_rwlock_t *rwlock);
void rwlock_unlock(pthread_rwlock_t *rwlock);
void rwlock_kill(pthread_rwlock_t *rwlock);

void cond_init(pthread_cond_t *cond);
void cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
void cond_signal(pthread_cond_t *cond);
void cond_broadcast(pthread_cond_t *cond);
void cond_kill(pthread_cond_t *cond);

#endif