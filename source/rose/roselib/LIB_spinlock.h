#pragma once

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Simple Math Macros
 * \{ */

#ifdef WITH_TBB
typedef uint32_t SpinLock;
#elif defined(_MSC_VER)
typedef volatile unsigned int SpinLock;
#else
typedef pthread_spinlock_t SpinLock;
#endif

void LIB_spin_init(SpinLock *spin);
void LIB_spin_lock(SpinLock *spin);
void LIB_spin_unlock(SpinLock *spin);
void LIB_spin_end(SpinLock *spin);

/* \} */

#ifdef __cplusplus
}
#endif
