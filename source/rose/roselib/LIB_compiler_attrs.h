#pragma once

#ifdef _MSC_VER

#include <sal.h>

/* -------------------------------------------------------------------- */
/** \name Microsoft C/C++ Compiler
 * \{ */

/** Indicates that the parameter is a null-terminated format string for use in a printf expression. */
#define ATTR_PRINTF_FORMAT _In_z_ _Printf_format_string_

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
/** \name Unsupported Compiler Attributes
 * \{ */

#if defined(_MSC_VER)
#  define ROSE_INLINE static __forceinline
#else
#  define ROSE_INLINE static inline __attribute__((always_inline)) __attribute__((__unused__))
#endif

/* /} */
