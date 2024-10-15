#ifndef LIB_MATH_BIT_H
#define LIB_MATH_BIT_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Base 2
 * \{ */

ROSE_INLINE uint32_t _lib_nextpow2_u32(uint32_t n);
ROSE_INLINE uint64_t _lib_nextpow2_u64(uint64_t n);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Bitset
 * \{ */

#if defined(_MSC_VER)
#	include <intrin.h>
#	define _lib_popcount_u32(n) __popcnt(n)
#	define _lib_popcount_u64(n) __popcnt64(n)
#elif defined(__has_builtin) && __has_builtin(__builtin_popcount)
#	define _lib_popcount_u32(n) __builtin_popcount(n)
#	define _lib_popcount_u64(n) __builtin_popcountll(n)
#else
ROSE_INLINE unsigned int _lib_software_popcount_u32(uint32_t n);
ROSE_INLINE unsigned int _lib_software_popcount_u64(uint64_t n);
#	define _lib_popcount_u32(n) _lib_software_popcount_u32(n)
#	define _lib_popcount_u64(n) _lib_software_popcount_u64(n)
#	warning "We do not know how to use popcnt fast with your compiler"
#endif

unsigned int _lib_forward_scan_u32(uint32_t n);
unsigned int _lib_forward_scan_u64(uint64_t n);

unsigned int _lib_forward_scan_clear_u32(uint32_t *n);
unsigned int _lib_forward_scan_clear_u64(uint64_t *n);

/** \} */

#ifdef __cplusplus
}
#endif

#include "intern/math_bit_inline.c"

#endif // LIB_MATH_BIT_H
