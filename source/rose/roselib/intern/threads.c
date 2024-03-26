#include "LIB_threads.h"

/* Mutex Lock */

void LIB_mutex_init(ThreadMutex *mutex) {
	pthread_mutex_init(mutex, NULL);
}

void LIB_mutex_end(ThreadMutex *mutex) {
	pthread_mutex_destroy(mutex);
}

ThreadMutex *LIB_mutex_alloc(void) {
	ThreadMutex *mutex = MEM_callocN(sizeof(ThreadMutex), "ThreadMutex");
	LIB_mutex_init(mutex);
	return mutex;
}

void LIB_mutex_free(ThreadMutex *mutex) {
	LIB_mutex_end(mutex);
	MEM_freeN(mutex);
}

void LIB_mutex_lock(ThreadMutex *mutex) {
	pthread_mutex_lock(mutex);
}
bool LIB_mutex_trylock(ThreadMutex *mutex) {
	return (pthread_mutex_trylock(mutex) == 0);
}
void LIB_mutex_unlock(ThreadMutex *mutex) {
	pthread_mutex_unlock(mutex);
}
