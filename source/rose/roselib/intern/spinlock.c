#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "LIB_spinlock.h"
#include "LIB_utildefines.h"

#ifdef WIN32
#	include <sys/timeb.h>
#	include <windows.h>
#elif defined(__APPLE__)
#	include <sys/sysctl.h>
#	include <sys/types.h>
#else
#	include <sys/time.h>
#	include <unistd.h>
#endif

void LIB_spin_init(SpinLock *spin) {
#if defined(_MSC_VER)
	*spin = 0;
#else
	pthread_spin_init(spin, 0);
#endif
}

void LIB_spin_lock(SpinLock *spin) {
#if defined(WITH_TBB)
	tbb::spin_mutex *spin_mutex = tbb_spin_mutex_cast(spin);
	spin_mutex->lock();
#elif defined(_MSC_VER)
	while (InterlockedExchangeAcquire(spin, 1)) {
		while (*spin) {
			/** Spin-lock hint for processors with hyper-threading. */
			YieldProcessor();
		}
	}
#else
	pthread_spin_lock(spin);
#endif
}

void LIB_spin_unlock(SpinLock *spin) {
#if defined(WITH_TBB)
	tbb::spin_mutex *spin_mutex = tbb_spin_mutex_cast(spin);
	spin_mutex->unlock();
#elif defined(_MSC_VER)
	_ReadWriteBarrier();
	*spin = 0;
#else
	pthread_spin_unlock(spin);
#endif
}

void LIB_spin_end(SpinLock *spin) {
#if defined(WITH_TBB)
	tbb::spin_mutex *spin_mutex = tbb_spin_mutex_cast(spin);
	spin_mutex->~spin_mutex();
#elif defined(_MSC_VER)
	/* Nothing to do, spin is a simple integer type. */
	UNUSED_VARS(spin);
#else
	pthread_spin_destroy(spin);
#endif
}
