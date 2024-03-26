#pragma once

#include <pthread.h>

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mutex Lock */

typedef pthread_mutex_t ThreadMutex;
#define LIB_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

void LIB_mutex_init(ThreadMutex *mutex);
void LIB_mutex_end(ThreadMutex *mutex);

ThreadMutex *LIB_mutex_alloc(void);
void LIB_mutex_free(ThreadMutex *mutex);

void LIB_mutex_lock(ThreadMutex *mutex);
bool LIB_mutex_trylock(ThreadMutex *mutex);
void LIB_mutex_unlock(ThreadMutex *mutex);

#ifdef __cplusplus
}
#endif
