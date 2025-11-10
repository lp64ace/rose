#ifndef MATH_BASE_INLINE_C
#define MATH_BASE_INLINE_C

#include "LIB_math_base.h"

#include "LIB_utildefines.h"

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE unsigned char unit_float_to_uchar_clamp(float val) {
	return (unsigned char)(((val <= 0.0f) ? 0 : ((val > (1.0f - 0.5f / 255.0f)) ? 255 : ((255.0f * val) + 0.5f))));
}
ROSE_INLINE unsigned short unit_float_to_ushort_clamp(float val) {
	return (unsigned short)((val >= 1.0f - 0.5f / 65535) ? 65535 : (val <= 0.0f) ? 0 : (val * 65535.0f + 0.5f));
}

#define unit_float_to_uchar_clamp_v3(v1, v2)          \
	{                                                 \
		(v1)[0] = unit_float_to_uchar_clamp((v2[0])); \
		(v1)[1] = unit_float_to_uchar_clamp((v2[1])); \
		(v1)[2] = unit_float_to_uchar_clamp((v2[2])); \
	}                                                 \
	((void)0)
#define unit_float_to_uchar_clamp_v4(v1, v2)          \
	{                                                 \
		(v1)[0] = unit_float_to_uchar_clamp((v2[0])); \
		(v1)[1] = unit_float_to_uchar_clamp((v2[1])); \
		(v1)[2] = unit_float_to_uchar_clamp((v2[2])); \
		(v1)[3] = unit_float_to_uchar_clamp((v2[3])); \
	}                                                 \
	((void)0)

#define _round_fl_impl(arg, ty)        \
	{                                  \
		return (ty)floorf(arg + 0.5f); \
	}
#define _round_db_impl(arg, ty)      \
	{                                \
		return (ty)floor(arg + 0.5); \
	}

ROSE_INLINE signed char round_fl_to_char(float a) {
	_round_fl_impl(a, signed char);
}
ROSE_INLINE unsigned char round_fl_to_uchar(float a) {
	_round_fl_impl(a, unsigned char);
}
ROSE_INLINE short round_fl_to_short(float a) {
	_round_fl_impl(a, short);
}
ROSE_INLINE unsigned short round_fl_to_ushort(float a) {
	_round_fl_impl(a, unsigned short);
}
ROSE_INLINE int round_fl_to_int(float a) {
	_round_fl_impl(a, int);
}
ROSE_INLINE unsigned int round_fl_to_uint(float a) {
	_round_fl_impl(a, unsigned int);
}

ROSE_INLINE signed char round_db_to_char(double a) {
	_round_db_impl(a, signed char);
}
ROSE_INLINE unsigned char round_db_to_uchar(double a) {
	_round_db_impl(a, unsigned char);
}
ROSE_INLINE short round_db_to_short(double a) {
	_round_db_impl(a, short);
}
ROSE_INLINE unsigned short round_db_to_ushort(double a) {
	_round_db_impl(a, unsigned short);
}
ROSE_INLINE int round_db_to_int(double a) {
	_round_db_impl(a, int);
}
ROSE_INLINE unsigned int round_db_to_uint(double a) {
	_round_db_impl(a, unsigned int);
}

#undef _round_fl_impl
#undef _round_db_impl

#define _round_clamp_fl_impl(arg, ty, min, max) \
	{                                           \
		float r = floorf(arg + 0.5f);           \
		if (r <= (float)min) {                  \
			return (ty)min;                     \
		}                                       \
		else if (r >= (float)max) {             \
			return (ty)max;                     \
		}                                       \
		else {                                  \
			return (ty)r;                       \
		}                                       \
	}

#define _round_clamp_db_impl(arg, ty, min, max) \
	{                                           \
		double r = floor(arg + 0.5);            \
		if (r <= (double)min) {                 \
			return (ty)min;                     \
		}                                       \
		else if (r >= (double)max) {            \
			return (ty)max;                     \
		}                                       \
		else {                                  \
			return (ty)r;                       \
		}                                       \
	}

ROSE_INLINE signed char round_fl_to_char_clamp(float a) {
	_round_clamp_fl_impl(a, signed char, SCHAR_MIN, SCHAR_MAX);
}
ROSE_INLINE unsigned char round_fl_to_uchar_clamp(float a) {
	_round_clamp_fl_impl(a, unsigned char, 0, UCHAR_MAX);
}
ROSE_INLINE short round_fl_to_short_clamp(float a) {
	_round_clamp_fl_impl(a, short, SHRT_MIN, SHRT_MAX);
}
ROSE_INLINE unsigned short round_fl_to_ushort_clamp(float a) {
	_round_clamp_fl_impl(a, unsigned short, 0, USHRT_MAX);
}
ROSE_INLINE int round_fl_to_int_clamp(float a) {
	_round_clamp_fl_impl(a, int, INT_MIN, INT_MAX);
}
ROSE_INLINE unsigned int round_fl_to_uint_clamp(float a) {
	_round_clamp_fl_impl(a, unsigned int, 0, UINT_MAX);
}

ROSE_INLINE signed char round_db_to_char_clamp(double a) {
	_round_clamp_db_impl(a, signed char, SCHAR_MIN, SCHAR_MAX);
}
ROSE_INLINE unsigned char round_db_to_uchar_clamp(double a) {
	_round_clamp_db_impl(a, unsigned char, 0, UCHAR_MAX);
}
ROSE_INLINE short round_db_to_short_clamp(double a) {
	_round_clamp_db_impl(a, short, SHRT_MIN, SHRT_MAX);
}
ROSE_INLINE unsigned short round_db_to_ushort_clamp(double a) {
	_round_clamp_db_impl(a, unsigned short, 0, USHRT_MAX);
}
ROSE_INLINE int round_db_to_int_clamp(double a) {
	_round_clamp_db_impl(a, int, INT_MIN, INT_MAX);
}
ROSE_INLINE unsigned int round_db_to_uint_clamp(double a) {
	_round_clamp_db_impl(a, unsigned int, 0, UINT_MAX);
}

#undef _round_clamp_fl_impl
#undef _round_clamp_db_impl

ROSE_INLINE float saacos(float fac) {
	if (fac <= -1.0f) {
		return (float)M_PI;
	}
	else if (fac >= 1.0f) {
		return 0.0f;
	}
	else {
		return acosf(fac);
	}
}

ROSE_INLINE float saasin(float fac) {
	if (fac <= -1.0f) {
		return (float)-M_PI_2;
	}
	else if (fac >= 1.0f) {
		return (float)M_PI_2;
	}
	else {
		return asinf(fac);
	}
}

ROSE_INLINE float increment_ulp(float value) {
	if (!isfinite(value)) {
		return value;
	}

	union {
		float f;
		uint i;
	} v;
	v.f = value;

	if (v.f > 0.0f) {
		v.i += 1;
	}
	else if (v.f < -0.0f) {
		v.i -= 1;
	}
	else {
		v.i = 0x00000001;
	}

	return v.f;
}

ROSE_INLINE float decrement_ulp(float value) {
	if (!isfinite(value)) {
		return value;
	}

	union {
		float f;
		uint i;
	} v;
	v.f = value;

	if (v.f > 0.0f) {
		v.i -= 1;
	}
	else if (v.f < -0.0f) {
		v.i += 1;
	}
	else {
		v.i = 0x80000001;
	}

	return v.f;
}

#ifdef __cplusplus
}
#endif

#endif	// MATH_BASE_INLINE_C
