#pragma once

#include "LIB_assert.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define ROSE_MATH_DO_INLINE 1

#if ROSE_MATH_DO_INLINE
#  ifdef _MSC_VER
#    define MINLINE static __forceinline
#    define MALWAYS_INLINE MINLINE
#  else
#    define MINLINE static inline
#    define MALWAYS_INLINE static inline __attribute__((always_inline)) __attribute__((unused))
#  endif
#else
#  define MINLINE
#  define MALWAYS_INLINE
#endif

/* Check for GCC push/pop pragma support. */
#ifdef __GNUC__
#  define ROSE_MATH_GCC_WARN_PRAGMA 1
#endif

#if defined(__cplusplus)
}
#endif
