#ifndef MATH_BIT_INLINE_C
#define MATH_BIT_INLINE_C

#include "LIB_assert.h"
#include "LIB_math_bit.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Pow 2
 * \{ */

ROSE_INLINE bool is_power_of_2_i(int x) {
	ROSE_assert(x >= 0);
	return (x & (x - 1)) == 0;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Base 2
 * \{ */

ROSE_INLINE uint32_t _lib_nextpow2_u32(uint32_t n) {
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n++;
	return n;
}
ROSE_INLINE uint64_t _lib_nextpow2_u64(uint64_t n) {
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	n++;
	return n;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conversion
 * \{ */

ROSE_INLINE float int_as_float(int i) {
	union {
		int i;
		float f;
	} u;
	u.i = i;
	return u.f;
}
ROSE_INLINE int float_as_int(float f) {
	union {
		int i;
		float f;
	} u;
	u.f = f;
	return u.i;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Bitset
 * \{ */

ROSE_INLINE unsigned int _lib_software_popcount_u32(uint32_t n) {
	n = (n & 0x55555555) + ((n >> 1) & 0x55555555);
	n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
	n = (n & 0x0f0f0f0f) + ((n >> 4) & 0x0f0f0f0f);
	n = (n & 0x00ff00ff) + ((n >> 8) & 0x00ff00ff);
	n = (n & 0x0000ffff) + ((n >> 16) & 0x0000ffff);
	return (unsigned int)n;
}
ROSE_INLINE unsigned int _lib_software_popcount_u64(uint64_t n) {
	n = (n & 0x5555555555555555) + ((n >> 1) & 0x5555555555555555);
	n = (n & 0x3333333333333333) + ((n >> 2) & 0x3333333333333333);
	n = (n & 0x0f0f0f0f0f0f0f0f) + ((n >> 4) & 0x0f0f0f0f0f0f0f0f);
	n = (n & 0x00ff00ff00ff00ff) + ((n >> 8) & 0x00ff00ff00ff00ff);
	n = (n & 0x0000ffff0000ffff) + ((n >> 16) & 0x0000ffff0000ffff);
	n = (n & 0x00000000ffffffff) + ((n >> 32) & 0x00000000ffffffff);
	return (unsigned int)n;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Float
 * \{ */

ROSE_INLINE float xor_fl(float x, int y) {
	return int_as_float(float_as_int(x) ^ y);
}

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// MATH_BIT_INLINE_C
