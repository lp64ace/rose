#pragma once

#include "lib_utildefines.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

/* copied from lib_utildefines.h */
#ifdef __GNUC__
#  define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#  define UNLIKELY(x) (x)
#endif

#define FLT_EPSILON     1e-7f

#ifndef M_PI
#  define M_PI 3.14159265358979323846 /* pi */
#endif
#ifndef M_PI_2
#  define M_PI_2 1.57079632679489661923 /* pi/2 */
#endif
#ifndef M_PI_4
#  define M_PI_4 0.78539816339744830962 /* pi/4 */
#endif
#ifndef M_SQRT2
#  define M_SQRT2 1.41421356237309504880 /* sqrt(2) */
#endif
#ifndef M_SQRT1_2
#  define M_SQRT1_2 0.70710678118654752440 /* 1/sqrt(2) */
#endif
#ifndef M_SQRT3
#  define M_SQRT3 1.73205080756887729352 /* sqrt(3) */
#endif
#ifndef M_SQRT1_3
#  define M_SQRT1_3 0.57735026918962576450 /* 1/sqrt(3) */
#endif
#ifndef M_1_PI
#  define M_1_PI 0.318309886183790671538 /* 1/pi */
#endif
#ifndef M_E
#  define M_E 2.7182818284590452354 /* e */
#endif
#ifndef M_LOG2E
#  define M_LOG2E 1.4426950408889634074 /* log_2 e */
#endif
#ifndef M_LOG10E
#  define M_LOG10E 0.43429448190325182765 /* log_10 e */
#endif
#ifndef M_LN2
#  define M_LN2 0.69314718055994530942 /* log_e 2 */
#endif
#ifndef M_LN10
#  define M_LN10 2.30258509299404568402 /* log_e 10 */
#endif

#if defined(__GNUC__)
#  define NAN_FLT __builtin_nanf("")
#else /* evil quiet NaN definition */
static const int NAN_INT = 0x7FC00000;
#  define NAN_FLT (*((float *)(&NAN_INT)))
#endif

inline float pow2f ( float x );
inline float pow3f ( float x );
inline float pow4f ( float x );
inline float pow7f ( float x );

inline float sqrt3f ( float f );
inline double sqrt3d ( double d );

inline float sqrtf_signed ( float f );

inline float saacosf ( float f );
inline float saasinf ( float f );
inline float sasqrtf ( float f );
inline float saacos ( float fac );
inline float saasin ( float fac );
inline float sasqrt ( float fac );

inline float interpf ( float a , float b , float t );
inline double interpd ( double a , double b , double t );

inline float ratiof ( float min , float max , float pos );
inline double ratiod ( double min , double max , double pos );

/**
 * Map a normalized value, i.e. from interval [0, 1] to interval [a, b].
 */
inline float scalenorm ( float a , float b , float x );
/**
 * Map a normalized value, i.e. from interval [0, 1] to interval [a, b].
 */
inline double scalenormd ( double a , double b , double x );

/* NOTE: Compilers will upcast all types smaller than int to int when performing arithmetic
 * operation. */

inline int square_s ( short a );
inline int square_uchar ( unsigned char a );
inline int cube_s ( short a );
inline int cube_uchar ( unsigned char a );

inline int square_i ( int a );
inline unsigned int square_uint ( unsigned int a );
inline float square_f ( float a );
inline double square_d ( double a );

inline int cube_i ( int a );
inline unsigned int cube_uint ( unsigned int a );
inline float cube_f ( float a );
inline double cube_d ( double a );

inline int clamp_i ( int value , int min , int max );
inline float clamp_f ( float value , float min , float max );
inline size_t clamp_z ( size_t value , size_t min , size_t max );

/**
 * Almost-equal for IEEE floats, using absolute difference method.
 *
 * \param max_diff: the maximum absolute difference.
 */
inline int compare_ff ( float a , float b , float max_diff );
/**
 * Almost-equal for IEEE floats, using their integer representation
 * (mixing ULP and absolute difference methods).
 *
 * \param max_diff: is the maximum absolute difference (allows to take care of the near-zero area,
 * where relative difference methods cannot really work).
 * \param max_ulps: is the 'maximum number of floats + 1'
 * allowed between \a a and \a b to consider them equal.
 *
 * \see https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
 */
inline int compare_ff_relative ( float a , float b , float max_diff , int max_ulps );
inline bool compare_threshold_relative ( float value1 , float value2 , float thresh );

inline float signf ( float f );
inline int signum_i_ex ( float a , float eps );
inline int signum_i ( float a );

/**
 * Used for zoom values.
 */
inline float power_of_2 ( float f );

/**
 * Returns number of (base ten) *significant* digits of integer part of given float
 * (negative in case of decimal-only floats, 0.01 returns -1 e.g.).
 */
inline int integer_digits_f ( float f );
/**
 * Returns number of (base ten) *significant* digits of integer part of given double
 * (negative in case of decimal-only floats, 0.01 returns -1 e.g.).
 */
inline int integer_digits_d ( double d );
inline int integer_digits_i ( int i );

/* These don't really fit anywhere but were being copied about a lot. */

inline int is_power_of_2_i ( int n );

inline unsigned int log2_floor_u ( unsigned int x );
inline unsigned int log2_ceil_u ( unsigned int x );

/**
 * Returns next (or previous) power of 2 or the input number if it is already a power of 2.
 */
inline int power_of_2_max_i ( int n );
inline int power_of_2_min_i ( int n );
inline unsigned int power_of_2_max_u ( unsigned int x );
inline unsigned int power_of_2_min_u ( unsigned int x );

/**
 * Integer division that rounds 0.5 up, particularly useful for color blending
 * with integers, to avoid gradual darkening when rounding down.
 */
inline int divide_round_i ( int a , int b );

/**
 * Integer division that returns the ceiling, instead of flooring like normal C division.
 */
inline unsigned int divide_ceil_u ( unsigned int a , unsigned int b );
inline uint64_t divide_ceil_ul ( uint64_t a , uint64_t b );

/**
 * Returns \a a if it is a multiple of \a b or the next multiple or \a b after \b a .
 */
inline unsigned int ceil_to_multiple_u ( unsigned int a , unsigned int b );
inline uint64_t ceil_to_multiple_ul ( uint64_t a , uint64_t b );

/**
 * modulo that handles negative numbers, works the same as Python's.
 */
inline int mod_i ( int i , int n );

/**
 * Round to closest even number, halfway cases are rounded away from zero.
 */
inline float round_to_even ( float f );

inline signed char round_fl_to_char ( float a );
inline unsigned char round_fl_to_uchar ( float a );
inline short round_fl_to_short ( float a );
inline unsigned short round_fl_to_ushort ( float a );
inline int round_fl_to_int ( float a );
inline unsigned int round_fl_to_uint ( float a );

inline signed char round_db_to_char ( double a );
inline unsigned char round_db_to_uchar ( double a );
inline short round_db_to_short ( double a );
inline unsigned short round_db_to_ushort ( double a );
inline int round_db_to_int ( double a );
inline unsigned int round_db_to_uint ( double a );

inline signed char round_fl_to_char_clamp ( float a );
inline unsigned char round_fl_to_uchar_clamp ( float a );
inline short round_fl_to_short_clamp ( float a );
inline unsigned short round_fl_to_ushort_clamp ( float a );
inline int round_fl_to_int_clamp ( float a );
inline unsigned int round_fl_to_uint_clamp ( float a );

inline signed char round_db_to_char_clamp ( double a );
inline unsigned char round_db_to_uchar_clamp ( double a );
inline short round_db_to_short_clamp ( double a );
inline unsigned short round_db_to_ushort_clamp ( double a );
inline int round_db_to_int_clamp ( double a );
inline unsigned int round_db_to_uint_clamp ( double a );

// source

inline float pow2f ( float x ) {
        return x * x;
}
inline float pow3f ( float x ) {
        return pow2f ( x ) * x;
}
inline float pow4f ( float x ) {
        return pow2f ( pow2f ( x ) );
}
inline float pow5f ( float x ) {
        return pow4f ( x ) * x;
}
inline float pow7f ( float x ) {
        return pow2f ( pow3f ( x ) ) * x;
}

inline float sqrt3f ( float f ) {
        if ( UNLIKELY ( f == 0.0f ) ) {
                return 0.0f;
        } else if ( UNLIKELY ( f < 0.0f ) ) {
                return -( float ) ( exp ( log ( -f ) / 3.0 ) );
        } else {
                return ( float ) ( exp ( log ( f ) / 3.0 ) );
        }
}

inline double sqrt3d ( double d ) {
        if ( UNLIKELY ( d == 0.0 ) ) {
                return 0.0;
        } else if ( UNLIKELY ( d < 0.0 ) ) {
                return -exp ( log ( -d ) / 3.0 );
        } else {
                return exp ( log ( d ) / 3.0 );
        }
}

inline float sqrtf_signed ( float f ) {
        return ( f >= 0.0f ) ? sqrtf ( f ) : -sqrtf ( -f );
}

inline float saacos ( float fac ) {
        if ( UNLIKELY ( fac <= -1.0f ) ) {
                return ( float ) M_PI;
        } else if ( UNLIKELY ( fac >= 1.0f ) ) {
                return 0.0f;
        } else {
                return acosf ( fac );
        }
}

inline float saasin ( float fac ) {
        if ( UNLIKELY ( fac <= -1.0f ) ) {
                return ( float ) -M_PI_2;
        } else if ( UNLIKELY ( fac >= 1.0f ) ) {
                return ( float ) M_PI_2;
        } else {
                return asinf ( fac );
        }
}

inline float sasqrt ( float fac ) {
        if ( UNLIKELY ( fac <= 0.0f ) ) {
                return 0.0f;
        } else {
                return sqrtf ( fac );
        }
}

inline float saacosf ( float fac ) {
        if ( UNLIKELY ( fac <= -1.0f ) ) {
                return ( float ) M_PI;
        } else if ( UNLIKELY ( fac >= 1.0f ) ) {
                return 0.0f;
        } else {
                return acosf ( fac );
        }
}

inline float saasinf ( float fac ) {
        if ( UNLIKELY ( fac <= -1.0f ) ) {
                return ( float ) -M_PI_2;
        } else if ( UNLIKELY ( fac >= 1.0f ) ) {
                return ( float ) M_PI_2;
        } else {
                return asinf ( fac );
        }
}

inline float sasqrtf ( float fac ) {
        if ( UNLIKELY ( fac <= 0.0f ) ) {
                return 0.0f;
        } else {
                return sqrtf ( fac );
        }
}

inline float interpf ( float target , float origin , float fac ) {
        return ( fac * target ) + ( 1.0f - fac ) * origin;
}

inline double interpd ( double target , double origin , double fac ) {
        return ( fac * target ) + ( 1.0f - fac ) * origin;
}

inline float ratiof ( float min , float max , float pos ) {
        float range = max - min;
        return range == 0 ? 0 : ( ( pos - min ) / range );
}

inline double ratiod ( double min , double max , double pos ) {
        double range = max - min;
        return range == 0 ? 0 : ( ( pos - min ) / range );
}

inline float scalenorm ( float a , float b , float x ) {
        assert ( x <= 1 && x >= 0 );
        return ( x * ( b - a ) ) + a;
}

inline double scalenormd ( double a , double b , double x ) {
        assert ( x <= 1 && x >= 0 );
        return ( x * ( b - a ) ) + a;
}

inline float power_of_2 ( float val ) {
        return ( float ) pow ( 2.0 , ceil ( log ( ( double ) val ) / M_LN2 ) );
}

inline int is_power_of_2_i ( int n ) {
        return ( n & ( n - 1 ) ) == 0;
}

inline int power_of_2_max_i ( int n ) {
        if ( is_power_of_2_i ( n ) ) {
                return n;
        }

        do {
                n = n & ( n - 1 );
        } while ( !is_power_of_2_i ( n ) );

        return n * 2;
}

inline int power_of_2_min_i ( int n ) {
        while ( !is_power_of_2_i ( n ) ) {
                n = n & ( n - 1 );
        }

        return n;
}

inline unsigned int power_of_2_max_u ( unsigned int x ) {
        x -= 1;
        x |= ( x >> 1 );
        x |= ( x >> 2 );
        x |= ( x >> 4 );
        x |= ( x >> 8 );
        x |= ( x >> 16 );
        return x + 1;
}

inline unsigned int power_of_2_min_u ( unsigned int x ) {
        x |= ( x >> 1 );
        x |= ( x >> 2 );
        x |= ( x >> 4 );
        x |= ( x >> 8 );
        x |= ( x >> 16 );
        return x - ( x >> 1 );
}

inline unsigned int log2_floor_u ( unsigned int x ) {
        return x <= 1 ? 0 : 1 + log2_floor_u ( x >> 1 );
}

inline unsigned int log2_ceil_u ( unsigned int x ) {
        if ( is_power_of_2_i ( ( int ) x ) ) {
                return log2_floor_u ( x );
        } else {
                return log2_floor_u ( x ) + 1;
        }
}

/* rounding and clamping */

#define _round_clamp_fl_impl(arg, ty, min, max) \
  { \
    float r = floorf(arg + 0.5f); \
    if (UNLIKELY(r <= (float)min)) { \
      return (ty)min; \
    } \
    else if (UNLIKELY(r >= (float)max)) { \
      return (ty)max; \
    } \
    else { \
      return (ty)r; \
    } \
  }

#define _round_clamp_db_impl(arg, ty, min, max) \
  { \
    double r = floor(arg + 0.5); \
    if (UNLIKELY(r <= (double)min)) { \
      return (ty)min; \
    } \
    else if (UNLIKELY(r >= (double)max)) { \
      return (ty)max; \
    } \
    else { \
      return (ty)r; \
    } \
  }

#define _round_fl_impl(arg, ty) \
  { \
    return (ty)floorf(arg + 0.5f); \
  }
#define _round_db_impl(arg, ty) \
  { \
    return (ty)floor(arg + 0.5); \
  }

inline signed char round_fl_to_char ( float a ) { _round_fl_impl ( a , signed char ) }
inline unsigned char round_fl_to_uchar ( float a ) { _round_fl_impl ( a , unsigned char ) }
inline short round_fl_to_short ( float a ) { _round_fl_impl ( a , short ) }
inline unsigned short round_fl_to_ushort ( float a ) { _round_fl_impl ( a , unsigned short ) }
inline int round_fl_to_int ( float a ) { _round_fl_impl ( a , int ) }
inline unsigned int round_fl_to_uint ( float a ) { _round_fl_impl ( a , unsigned int ) }

inline signed char round_db_to_char ( double a ) { _round_db_impl ( a , signed char ) }
inline unsigned char round_db_to_uchar ( double a ) { _round_db_impl ( a , unsigned char ) }
inline short round_db_to_short ( double a ) { _round_db_impl ( a , short ) }
inline unsigned short round_db_to_ushort ( double a ) { _round_db_impl ( a , unsigned short ) }
inline int round_db_to_int ( double a ) { _round_db_impl ( a , int ) }
inline unsigned int round_db_to_uint ( double a ) { _round_db_impl ( a , unsigned int ) }

#undef _round_fl_impl
#undef _round_db_impl

inline signed char round_fl_to_char_clamp ( float a ) {
        _round_clamp_fl_impl ( a , signed char , SCHAR_MIN , SCHAR_MAX )
}

inline unsigned char round_fl_to_uchar_clamp ( float a ) {
        _round_clamp_fl_impl ( a , unsigned char , 0 , UCHAR_MAX )
}
inline short round_fl_to_short_clamp ( float a ) {
        _round_clamp_fl_impl ( a , short , SHRT_MIN , SHRT_MAX )
}
inline unsigned short round_fl_to_ushort_clamp ( float a ) {
        _round_clamp_fl_impl ( a , unsigned short , 0 , USHRT_MAX )
}
inline int round_fl_to_int_clamp ( float a ) {
        _round_clamp_fl_impl ( a , int , INT_MIN , INT_MAX )
}
inline unsigned int round_fl_to_uint_clamp ( float a ) {
        _round_clamp_fl_impl ( a , unsigned int , 0 , UINT_MAX )
}
inline signed char round_db_to_char_clamp ( double a ) {
        _round_clamp_db_impl ( a , signed char , SCHAR_MIN , SCHAR_MAX )
}
inline unsigned char round_db_to_uchar_clamp ( double a ) {
        _round_clamp_db_impl ( a , unsigned char , 0 , UCHAR_MAX )
}
inline short round_db_to_short_clamp ( double a ) {
        _round_clamp_db_impl ( a , short , SHRT_MIN , SHRT_MAX )
}
inline unsigned short round_db_to_ushort_clamp ( double a ) {
        _round_clamp_db_impl ( a , unsigned short , 0 , USHRT_MAX )
}
inline int round_db_to_int_clamp ( double a ) {
        _round_clamp_db_impl ( a , int , INT_MIN , INT_MAX )
}
inline unsigned int round_db_to_uint_clamp ( double a ) {
        _round_clamp_db_impl ( a , unsigned int , 0 , UINT_MAX )
}

#undef _round_clamp_fl_impl
#undef _round_clamp_db_impl

inline float round_to_even ( float f ) {
        return roundf ( f * 0.5f ) * 2.0f;
}

inline int divide_round_i ( int a , int b ) {
        return ( 2 * a + b ) / ( 2 * b );
}

/**
* Integer division that floors negative result.
* \note This works like Python's int division.
*/
inline int divide_floor_i ( int a , int b ) {
        int d = a / b;
        int r = a % b; /* Optimizes into a single division. */
        return r ? d - ( ( a < 0 ) ^ ( b < 0 ) ) : d;
}

/**
* Integer division that returns the ceiling, instead of flooring like normal C division.
*/
inline unsigned int divide_ceil_u ( unsigned int a , unsigned int b ) {
        return ( a + b - 1 ) / b;
}

inline uint64_t divide_ceil_ul ( uint64_t a , uint64_t b ) {
        return ( a + b - 1 ) / b;
}

/**
* Returns \a a if it is a multiple of \a b or the next multiple or \a b after \b a .
*/
inline unsigned int ceil_to_multiple_u ( unsigned int a , unsigned int b ) {
        return divide_ceil_u ( a , b ) * b;
}

inline uint64_t ceil_to_multiple_ul ( uint64_t a , uint64_t b ) {
        return divide_ceil_ul ( a , b ) * b;
}

inline int mod_i ( int i , int n ) {
        return ( i % n + n ) % n;
}

inline float fractf ( float a ) {
        return a - floorf ( a );
}

/* Adapted from `godot-engine` math_funcs.h. */
inline float wrapf ( float value , float max , float min ) {
        float range = max - min;
        return ( range != 0.0f ) ? value - ( range * floorf ( ( value - min ) / range ) ) : min;
}

inline float pingpongf ( float value , float scale ) {
        if ( scale == 0.0f ) {
                return 0.0f;
        }
        return fabsf ( fractf ( ( value - scale ) / ( scale * 2.0f ) ) * scale * 2.0f - scale );
}

/* Square. */

inline int square_s ( short a ) {
        return a * a;
}

inline int square_i ( int a ) {
        return a * a;
}

inline unsigned int square_uint ( unsigned int a ) {
        return a * a;
}

inline int square_uchar ( unsigned char a ) {
        return a * a;
}

inline float square_f ( float a ) {
        return a * a;
}

inline double square_d ( double a ) {
        return a * a;
}

/* Cube. */

inline int cube_s ( short a ) {
        return a * a * a;
}

inline int cube_i ( int a ) {
        return a * a * a;
}

inline unsigned int cube_uint ( unsigned int a ) {
        return a * a * a;
}

inline int cube_uchar ( unsigned char a ) {
        return a * a * a;
}

inline float cube_f ( float a ) {
        return a * a * a;
}

inline double cube_d ( double a ) {
        return a * a * a;
}

/* Min/max */

inline float min_ff ( float a , float b ) {
        return ( a < b ) ? a : b;
}
inline float max_ff ( float a , float b ) {
        return ( a > b ) ? a : b;
}
/* See: https://www.iquilezles.org/www/articles/smin/smin.htm. */
inline float smoothminf ( float a , float b , float c ) {
        if ( c != 0.0f ) {
                float h = max_ff ( c - fabsf ( a - b ) , 0.0f ) / c;
                return min_ff ( a , b ) - h * h * h * c * ( 1.0f / 6.0f );
        } else {
                return min_ff ( a , b );
        }
}

inline float smoothstep ( float edge0 , float edge1 , float x ) {
        float result;
        if ( x < edge0 ) {
                result = 0.0f;
        } else if ( x >= edge1 ) {
                result = 1.0f;
        } else {
                float t = ( x - edge0 ) / ( edge1 - edge0 );
                result = ( 3.0f - 2.0f * t ) * ( t * t );
        }
        return result;
}

inline int clamp_i ( int value , int min , int max ) {
        return ROSE_MIN ( ROSE_MAX ( value , min ) , max );
}

inline float clamp_f ( float value , float min , float max ) {
        if ( value > max ) {
                return max;
        } else if ( value < min ) {
                return min;
        }
        return value;
}

inline size_t clamp_z ( size_t value , size_t min , size_t max ) {
        return ROSE_MIN ( ROSE_MAX ( value , min ) , max );
}

inline int compare_ff ( float a , float b , const float max_diff ) {
        return fabsf ( a - b ) <= max_diff;
}

inline int compare_ff_relative ( float a , float b , const float max_diff , const int max_ulps ) {
        union {
                float f;
                int i;
        } ua , ub;

        assert ( sizeof ( float ) == sizeof ( int ) );
        assert ( max_ulps < ( 1 << 22 ) );

        if ( fabsf ( a - b ) <= max_diff ) {
                return 1;
        }

        ua.f = a;
        ub.f = b;

        /* Important to compare sign from integers, since (-0.0f < 0) is false
                * (though this shall not be an issue in common cases)... */
        return ( ( ua.i < 0 ) != ( ub.i < 0 ) ) ? 0 : ( abs ( ua.i - ub.i ) <= max_ulps ) ? 1 : 0;
}

inline bool compare_threshold_relative ( const float value1 , const float value2 , const float thresh ) {
        const float abs_diff = fabsf ( value1 - value2 );
        /* Avoid letting the threshold get too small just because the values happen to be close to zero.
                */
        if ( fabsf ( value2 ) < 1 ) {
                return abs_diff > thresh;
        }
        /* Using relative threshold in general. */
        return abs_diff > thresh * fabsf ( value2 );
}

inline float signf ( float f ) {
        return ( f < 0.0f ) ? -1.0f : 1.0f;
}

inline float compatible_signf ( float f ) {
        if ( f > 0.0f ) {
                return 1.0f;
        }
        if ( f < 0.0f ) {
                return -1.0f;
        } else {
                return 0.0f;
        }
}

inline int signum_i_ex ( float a , float eps ) {
        if ( a > eps ) {
                return 1;
        }
        if ( a < -eps ) {
                return -1;
        } else {
                return 0;
        }
}

inline int signum_i ( float a ) {
        if ( a > 0.0f ) {
                return 1;
        }
        if ( a < 0.0f ) {
                return -1;
        } else {
                return 0;
        }
}

inline int integer_digits_f ( const float f ) {
        return ( f == 0.0f ) ? 0 : ( int ) floor ( log10 ( fabs ( f ) ) ) + 1;
}

inline int integer_digits_d ( const double d ) {
        return ( d == 0.0 ) ? 0 : ( int ) floor ( log10 ( fabs ( d ) ) ) + 1;
}

inline int integer_digits_i ( const int i ) {
        return ( int ) log10 ( ( double ) i ) + 1;
}
