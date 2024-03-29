#pragma once

#include "LIB_math_base.h"

#ifdef __cplusplus
extern "C" {
#endif

MINLINE float safe_divide(float a, float b);
MINLINE float safe_modf(float a, float b);
MINLINE float safe_logf(float a, float base);
MINLINE float safe_sqrtf(float a);
MINLINE float safe_inverse_sqrtf(float a);
MINLINE float safe_asinf(float a);
MINLINE float safe_acosf(float a);
MINLINE float safe_powf(float base, float exponent);

#ifdef __cplusplus
}
#endif

#if ROSE_MATH_DO_INLINE
#	include "intern/math_base_safe_inline.c"
#endif
