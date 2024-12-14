#ifndef LIB_THREAD_H
#define LIB_THREAD_H

#include <pthread.h>

#include "LIB_sys_types.h"

#define ROSE_MAX_THREADS 1024

#ifdef __cplusplus
extern "C" {
#endif

struct ListBase;

/* -------------------------------------------------------------------- */
/** \name Thread Pool
 * \{ */

void LIB_threadapi_init(void);
void LIB_threadapi_exit(void);

/**
 * \param tot: When 0 only initializes malloc mutex in a safe way (see sequence.c)
 * problem otherwise: scene render will kill of the mutex!
 */
void LIB_threadpool_init(struct ListBase *threadbase, void *(*do_thread)(void *), int tot);

/**
 * Amount of available threads.
 */
int LIB_available_threads(struct ListBase *threadbase);

/**
 * Returns thread number, for sample paters or threadsafe tables.
 */
int LIB_threadpool_available_thread_index(struct ListBase *threadbase);
void LIB_threadpool_insert(struct ListBase *threadbase, void *callerdata);
void LIB_threadpool_remove(struct ListBase *threadbase, void *callerdata);
void LIB_threadpool_remove_index(struct ListBase *threadbase, int index);
void LIB_threadpool_clear(struct ListBase *threadbase);
void LIB_threadpool_end(struct ListBase *threadbase);
int LIB_thread_is_main(void);

/** \} */

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

/* -------------------------------------------------------------------- */
/** \name Thread Queue
 * \{ */

typedef struct ThreadQueue ThreadQueue;

ThreadQueue *LIB_thread_queue_init(void);
void LIB_thread_queue_free(ThreadQueue *queue);

void LIB_thread_queue_push(ThreadQueue *queue, void *work);
void *LIB_thread_queue_pop(ThreadQueue *queue);
void *LIB_thread_queue_pop_timeout(ThreadQueue *queue, int ms);
int LIB_thread_queue_len(ThreadQueue *queue);
bool LIB_thread_queue_is_empty(ThreadQueue *queue);

void LIB_thread_queue_wait_finish(ThreadQueue *queue);
void LIB_thread_queue_nowait(ThreadQueue *queue);

/** \} */

/* -------------------------------------------------------------------- */
/** \name System Information
 * \{ */

size_t LIB_system_thread_count();

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_THREAD_H
