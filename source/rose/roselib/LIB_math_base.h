#ifndef LIB_MATH_BASE_H
#define LIB_MATH_BASE_H

#include "LIB_sys_types.h"

#include <math.h>

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

uint32_t divide_ceil_u32(uint32_t a, uint32_t b);
uint32_t ceil_to_multiple_u32(uint32_t a, uint32_t b);

uint64_t divide_ceil_u64(uint64_t a, uint64_t b);
uint64_t ceil_to_multiple_u64(uint64_t a, uint64_t b);

float clampf(float x, float low, float high);

#ifdef __cplusplus
}
#endif

#endif	// LIB_MATH_BASE_H
