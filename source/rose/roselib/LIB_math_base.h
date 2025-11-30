#ifndef LIB_MATH_BASE_H
#define LIB_MATH_BASE_H

#include "LIB_utildefines.h"

#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#	define M_PI 3.14159265358979323846 /* pi */
#endif
#ifndef M_PI_2
#	define M_PI_2 1.57079632679489661923 /* pi/2 */
#endif
#ifndef M_PI_4
#	define M_PI_4 0.78539816339744830962 /* pi/4 */
#endif
#ifndef M_SQRT2
#	define M_SQRT2 1.41421356237309504880 /* sqrt(2) */
#endif
#ifndef M_SQRT1_2
#	define M_SQRT1_2 0.70710678118654752440 /* 1/sqrt(2) */
#endif
#ifndef M_SQRT3
#	define M_SQRT3 1.73205080756887729352 /* sqrt(3) */
#endif
#ifndef M_SQRT1_3
#	define M_SQRT1_3 0.57735026918962576450 /* 1/sqrt(3) */
#endif
#ifndef M_1_PI
#	define M_1_PI 0.318309886183790671538 /* 1/pi */
#endif
#ifndef M_E
#	define M_E 2.7182818284590452354 /* e */
#endif
#ifndef M_LOG2E
#	define M_LOG2E 1.4426950408889634074 /* log_2 e */
#endif
#ifndef M_LOG10E
#	define M_LOG10E 0.43429448190325182765 /* log_10 e */
#endif
#ifndef M_LN2
#	define M_LN2 0.69314718055994530942 /* log_e 2 */
#endif
#ifndef M_LN10
#	define M_LN10 2.30258509299404568402 /* log_e 10 */
#endif

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE double sqrt3d(double d);

ROSE_INLINE float interpf(float target, float origin, float t);
ROSE_INLINE double interpd(double target, double origin, double t);

int32_t divide_ceil_i32(int32_t a, int32_t b);
int32_t divide_round_i32(int32_t a, int32_t b);
int32_t ceil_to_multiple_i32(int32_t a, int32_t b);

int64_t divide_ceil_i64(int64_t a, int64_t b);
int64_t divide_round_i64(int64_t a, int64_t b);
int64_t ceil_to_multiple_i64(int64_t a, int64_t b);

uint32_t divide_ceil_u32(uint32_t a, uint32_t b);
uint32_t divide_round_u32(uint32_t a, uint32_t b);
uint32_t ceil_to_multiple_u32(uint32_t a, uint32_t b);

uint64_t divide_ceil_u64(uint64_t a, uint64_t b);
uint64_t divide_round_u64(uint64_t a, uint64_t b);
uint64_t ceil_to_multiple_u64(uint64_t a, uint64_t b);

float clampf(float x, float low, float high);

ROSE_INLINE signed char round_fl_to_char(float a);
ROSE_INLINE unsigned char round_fl_to_uchar(float a);
ROSE_INLINE short round_fl_to_short(float a);
ROSE_INLINE unsigned short round_fl_to_ushort(float a);
ROSE_INLINE int round_fl_to_int(float a);
ROSE_INLINE unsigned int round_fl_to_uint(float a);

ROSE_INLINE signed char round_db_to_char(double a);
ROSE_INLINE unsigned char round_db_to_uchar(double a);
ROSE_INLINE short round_db_to_short(double a);
ROSE_INLINE unsigned short round_db_to_ushort(double a);
ROSE_INLINE int round_db_to_int(double a);
ROSE_INLINE unsigned int round_db_to_uint(double a);

ROSE_INLINE signed char round_fl_to_char_clamp(float a);
ROSE_INLINE unsigned char round_fl_to_uchar_clamp(float a);
ROSE_INLINE short round_fl_to_short_clamp(float a);
ROSE_INLINE unsigned short round_fl_to_ushort_clamp(float a);
ROSE_INLINE int round_fl_to_int_clamp(float a);
ROSE_INLINE unsigned int round_fl_to_uint_clamp(float a);

ROSE_INLINE signed char round_db_to_char_clamp(double a);
ROSE_INLINE unsigned char round_db_to_uchar_clamp(double a);
ROSE_INLINE short round_db_to_short_clamp(double a);
ROSE_INLINE unsigned short round_db_to_ushort_clamp(double a);
ROSE_INLINE int round_db_to_int_clamp(double a);
ROSE_INLINE unsigned int round_db_to_uint_clamp(double a);

ROSE_INLINE float saacos(float fac);
ROSE_INLINE float saasin(float fac);

/**
 * Increment the given float to the next representable floating point value in
 * the positive direction.
 *
 * Infinities and NaNs are left untouched. Subnormal numbers are handled
 * correctly, as is crossing zero (i.e. 0 and -0 are considered a single value,
 * and progressing past zero continues on to the positive numbers).
 */
ROSE_INLINE float increment_ulp(float value);

/**
 * Decrement the given float to the next representable floating point value in
 * the negative direction.
 *
 * Infinities and NaNs are left untouched. Subnormal numbers are handled
 * correctly, as is zero (i.e. 0 and -0 are considered a single value, and
 * progressing past zero continues on to the negative numbers).
 */
ROSE_INLINE float decrement_ulp(float value);

#ifdef __cplusplus
}
#endif

#include "intern/math_base_inline.c"

#endif	// LIB_MATH_BASE_H
