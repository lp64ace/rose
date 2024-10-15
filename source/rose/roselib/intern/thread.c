#include "MEM_guardedalloc.h"

#include "LIB_thread.h"
#include "LIB_utildefines.h"

/* -------------------------------------------------------------------- */
/** \name Mutex Lock
 * \{ */

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
	return pthread_mutex_trylock(mutex) == 0;
}
void LIB_mutex_unlock(ThreadMutex *mutex) {
	pthread_mutex_unlock(mutex);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Spin Lock
 * \{ */

void LIB_spin_init(SpinLock *spin) {
#if defined(__APPLE__)
	LIB_mutex_init(spin);
#elif defined(_MSC_VER)
	*spin = 0;
#else
	pthread_spin_init(spin, 0);
#endif
}

void LIB_spin_lock(SpinLock *spin) {
#if defined(__APPLE__)
	LIB_mutex_lock(spin);
#elif defined(_MSC_VER)
#	if defined(_M_ARM64)
	// InterlockedExchangeAcquire takes a long arg on MSVC ARM64
	ROSE_STATIC_ASSERT(sizeof(long) == sizeof(SpinLock));
	while (InterlockedExchangeAcquire((volatile long *)spin, 1)) {
#	else
	while (InterlockedExchangeAcquire(spin, 1)) {
#	endif
		while (*spin) {
			/* Spin-lock hint for processors with hyper-threading. */
			YieldProcessor();
		}
	}
#else
	pthread_spin_lock(spin);
#endif
}

void LIB_spin_unlock(SpinLock *spin) {
#if defined(__APPLE__)
	LIB_mutex_unlock(spin);
#elif defined(_MSC_VER)
	_ReadWriteBarrier();
	*spin = 0;
#else
	pthread_spin_unlock(spin);
#endif
}

void LIB_spin_end(SpinLock *spin) {
#if defined(__APPLE__)
	LIB_mutex_end(spin);
#elif defined(_MSC_VER)
	/* Nothing to do, spin is a simple integer type. */
	UNUSED_VARS(spin);
#else
	pthread_spin_destroy(spin);
#endif
}

/** \} */
