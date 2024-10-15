#ifndef ALGORITHM_RABIN_KARP_H
#define ALGORITHM_RABIN_KARP_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Simple Rabin Karp Hashing
 * \{ */

int64_t rabin_karp_rolling_hash_push(int64_t hash, const void *itr, size_t length);
int64_t rabin_karp_rolling_hash_roll(int64_t hash, const void *itr, size_t length);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Advanced Rabin Karp Hashing
 * \{ */

int64_t rabin_karp_rolling_hash_push_ex(int64_t hash, const void *itr, size_t length, ptrdiff_t step);
int64_t rabin_karp_rolling_hash_roll_ex(int64_t hash, const void *itr, size_t length, ptrdiff_t step);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // ALGORITHM_RABIN_KARP_H
