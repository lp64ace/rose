#ifndef LIB_THREAD_H
#define LIB_THREAD_H

#include <pthread.h>

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Mutex Lock
 * \{ */

/* Type definition for thread mutex using pthreads library. */
typedef pthread_mutex_t ThreadMutex;
#define ROSE_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER

/* Initialize the mutex to a valid state. This must be called before the mutex can be locked or unlocked. */
void LIB_mutex_init(ThreadMutex *mutex);
/* Destroy the mutex, releasing any resources associated with it. */
void LIB_mutex_end(ThreadMutex *mutex);

/* Allocate memory for a new mutex and initialize it. Returns a pointer to the initialized mutex. */
ThreadMutex *LIB_mutex_alloc(void);
/* Free the memory associated with the mutex and destroy it. */
void LIB_mutex_free(ThreadMutex *mutex);

/**
 * Lock the mutex.
 *
 * If the mutex is already locked by another thread, the calling thread will block until the
 * mutex becomes available.
 */
void LIB_mutex_lock(ThreadMutex *mutex);
/**
 * Try to lock the mutex.
 *
 * If the mutex is already locked, this function returns false immediately, without blocking.
 * If successful, it returns true.
 */
bool LIB_mutex_trylock(ThreadMutex *mutex);
/**
 * Unlock the mutex.
 *
 * Unlock the mutex allowing other threads to acquire it.
 * Must only be called by the thread that locked it.
 */
void LIB_mutex_unlock(ThreadMutex *mutex);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Spin Lock
 * \{ */

#if defined(__APPLE__)
typedef ThreadMutex SpinLock;
#elif defined(_MSC_VER)
typedef volatile unsigned int SpinLock;
#else
typedef pthread_spinlock_t SpinLock;
#endif

/**
 * Initialize the spinlock to a valid state.
 *
 * This function prepares the spinlock for use.
 * It must be called before the spinlock is locked or unlocked.
 */
void LIB_spin_init(SpinLock *spin);

/**
 * Acquire the spinlock.
 *
 * The thread will repeatedly check (spin) until the spinlock becomes available.
 * Unlike a mutex, the thread does not sleep but continuously checks,
 * making this useful for short critical sections where the lock is expected to be held briefly.
 */
void LIB_spin_lock(SpinLock *spin);

/**
 * Release the spinlock, allowing other threads to acquire it.
 *
 * This function must only be called by the thread that currently holds the lock.
 * Failure to release the spinlock can result in a deadlock where other threads are stuck waiting
 * for the lock indefinitely.
 */
void LIB_spin_unlock(SpinLock *spin);

/**
 * Destroy the spinlock and clean up any resources associated with it.
 *
 * This should be called when the spinlock is no longer needed to ensure that any system resources,
 * such as memory, are properly freed.
 */
void LIB_spin_end(SpinLock *spin);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_THREAD_H
