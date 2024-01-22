#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NDEBUG
#	define WITH_ASSERT_ABORT
#endif

void _LIB_assert_print_pos(const char *file, int line, const char *function, const char *id);
void _LIB_assert_print_extra(const char *str);
void _LIB_assert_abort(void);
void _LIB_assert_backtrace(void);
void _LIB_assert_unreachable_print(const char *file, int line, const char *function);

#ifdef _MSC_VER
#	include <crtdbg.h>
#endif

#ifndef NDEBUG

/* -------------------------------------------------------------------- */
/** \name Print Position
 * \{ */

#  if defined(__GNUC__)
#    define _ROSE_ASSERT_PRINT_POS(a) _LIB_assert_print_pos(__FILE__, __LINE__, __func__, #    a)
#  elif defined(_MSC_VER)
#    define _ROSE_ASSERT_PRINT_POS(a) _LIB_assert_print_pos(__FILE__, __LINE__, __func__, #    a)
#  else
#    define _ROSE_ASSERT_PRINT_POS(a) _LIB_assert_print_pos(__FILE__, __LINE__, "<?>", #    a)
#  endif

/* \} */

/* -------------------------------------------------------------------- */
/** \name Assert Abort
 * \{ */

#  ifdef WITH_ASSERT_ABORT
#    define _ROSE_ASSERT_ABORT _LIB_assert_abort
#  else
#    define _ROSE_ASSERT_ABORT() (void)0
#  endif

/* \} */

/* -------------------------------------------------------------------- */
/** \name Assert
 * \{ */

#define ROSE_assert(a) \
	(void)((!(a)) ? ((_LIB_assert_backtrace(), _ROSE_ASSERT_PRINT_POS(a), _ROSE_ASSERT_ABORT(), NULL)) : NULL)

/* \} */

/* -------------------------------------------------------------------- */
/** \name Assert With Message
 * \{ */

#define ROSE_assert_msg(a, msg) \
	(void)((!(a)) ? ((_LIB_assert_backtrace(), \
					  _ROSE_ASSERT_PRINT_POS(a), \
					  _LIB_assert_print_extra(msg), \
					  _ROSE_ASSERT_ABORT(), \
					  NULL)) : \
					  NULL)

/* \} */

#else
	
/* -------------------------------------------------------------------- */
/** \name Assert
 * \{ */

#  define ROSE_assert(a) ((void)0)

/* \} */

/* -------------------------------------------------------------------- */
/** \name Assert With Message
 * \{ */

#  define ROSE_assert_msg(a, msg) ((void)0)

/* \} */

#endif

/* -------------------------------------------------------------------- */
/** \name Static Assert
 * \{ */

#if defined(__cplusplus)
#	define ROSE_STATIC_ASSERT(a, msg) static_assert(a, msg);
#elif defined(_MSC_VER)
#	if !defined(__clang__)
#		define ROSE_STATIC_ASSERT(a, msg) static_assert(a, msg);
#	else
#		define ROSE_STATIC_ASSERT(a, msg) _STATIC_ASSERT(a);
#	endif
#elif defined(__COVERITY__)
#	define ROSE_STATIC_ASSERT(a, msg)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#	define ROSE_STATIC_ASSERT(a, msg) _Static_assert(a, msg);
#else
#	define ROSE_STATIC_ASSERT(a, msg)
#endif

/* \} */

/* -------------------------------------------------------------------- */
/** \name Static Assert Align
 * \{ */

#define ROSE_STATIC_ASSERT_ALIGN(st, align) \
	ROSE_STATIC_ASSERT((sizeof(st) % (align) == 0), "Structure must be strictly aligned")

/* \} */

#ifdef __cplusplus
}
#endif
