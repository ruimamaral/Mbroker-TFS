#include "betterassert.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

//Mutexes
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
// RWLocks
void rwlock_init(pthread_rwlock_t *rwlock) {
	if (pthread_rwlock_init(rwlock, NULL) != 0) {
		exit(EXIT_FAILURE);
	}
}
void rwlock_rdlock(pthread_rwlock_t *rwlock) {
	if (pthread_rwlock_rdlock(rwlock) != 0) {
		exit(EXIT_FAILURE);
	}
}
void rwlock_wrlock(pthread_rwlock_t *rwlock) {
	if (pthread_rwlock_wrlock(rwlock) != 0) {
		exit(EXIT_FAILURE);
	}
}
void rwlock_unlock(pthread_rwlock_t *rwlock) {
	if (pthread_rwlock_unlock(rwlock) != 0) {
		exit(EXIT_FAILURE);
	}
}
void rwlock_kill(pthread_rwlock_t *rwlock) {
	if (pthread_rwlock_destroy(rwlock) != 0) {
		exit(EXIT_FAILURE);
	}
}
// Cond vars
void cond_init(pthread_cond_t *cond) {
	ALWAYS_ASSERT(pthread_cond_init(cond,NULL) == 0, "cond_init fail");
}
void cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
	ALWAYS_ASSERT(pthread_cond_wait(cond, mutex) == 0, "cond_wait fail");
}
void cond_signal(pthread_cond_t *cond) {
	ALWAYS_ASSERT(pthread_cond_signal(cond) == 0, "cond_signal fail");
}
void cond_broadcast(pthread_cond_t *cond) {
	ALWAYS_ASSERT(pthread_cond_broadcast(
			cond) == 0, "cond_broadcast fail");
}
void cond_kill(pthread_cond_t *cond) {
	ALWAYS_ASSERT(pthread_cond_destroy(cond) == 0, "cond_destroy fail");
}

