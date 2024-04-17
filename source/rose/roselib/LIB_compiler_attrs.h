#pragma once

#include <stdlib.h>

#ifdef _MSC_VER

#	include <sal.h>

/* -------------------------------------------------------------------- */
/** \name Microsoft C/C++ Compiler
 * \{ */

/** Indicates that the parameter is a null-terminated format string for use in a printf expression. */
#	define ATTR_PRINTF_FORMAT _In_z_ _Printf_format_string_

/* /} */

#endif

/* -------------------------------------------------------------------- */
/** \name Unsupported Compiler Attributes
 * \{ */

#if !defined(ATTR_PRINTF_FORMAT)
/** Indicates that the parameter is a null-terminated format string for use in a printf expression. */
#	define ATTR_PRINTF_FORMAT
#endif

/* /} */

/* -------------------------------------------------------------------- */
/** \name Inline Attributes
 * \{ */

#if defined(_MSC_VER)
#	define ROSE_INLINE static __forceinline
#else
#	define ROSE_INLINE static inline __attribute__((always_inline)) __attribute__((__unused__))
#endif

#if defined(_MSC_VER)
#	define ROSE_NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
#	define ROSE_NOINLINE __attribute__((noinline))
#else
#	define ROSE_NOINLINE
#endif

#define ROSE_MATH_DO_INLINE 1

/* /} */

/* -------------------------------------------------------------------- */
/** \name Attr Fallthrough
 * \{ */

#define ATTR_FALLTHROUGH

/* /} */

/* -------------------------------------------------------------------- */
/** \name Expression Optimization
 * \{ */

#ifdef __GNUC__
#	define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#	define UNLIKELY(x) (x)
#endif

#ifdef __GNUC__
#	define LIKELY(x) __builtin_expect(!!(x), 1)
#else
#	define LIKELY(x) (x)
#endif

/* /} */
