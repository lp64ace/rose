#ifndef LIB_ASSERT_H
#define LIB_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#define WITH_ASSERT_ABORT

void _ROSE_assert_print_pos(const char *file, int line, const char *function, const char *id);
void _ROSE_assert_print_extra(const char *str);
void _ROSE_assert_abort(void);
void _ROSE_assert_backtrace(void);
void _ROSE_assert_unreachable_print(const char *file, int line, const char *function);

#ifdef _MSC_VER
#	include <crtdbg.h>
#endif

#ifndef NDEBUG

/* -------------------------------------------------------------------- */
/** \name Print Position
 * \{ */

#	if defined(__GNUC__)
#		define _ROSE_assert_PRINT_POS(a) _ROSE_assert_print_pos(__FILE__, __LINE__, __func__, #a)
#	elif defined(_MSC_VER)
#		define _ROSE_assert_PRINT_POS(a) _ROSE_assert_print_pos(__FILE__, __LINE__, __func__, #a)
#	else
#		define _ROSE_assert_PRINT_POS(a) _ROSE_assert_print_pos(__FILE__, __LINE__, "<?>", #a)
#	endif

/** \} */

/* -------------------------------------------------------------------- */
/** \name Assert Abort
 * \{ */

#	ifdef WITH_ASSERT_ABORT
#		define _ROSE_assert_ABORT _ROSE_assert_abort
#	else
#		define _ROSE_assert_ABORT() (void)0
#	endif

/** \} */

/* -------------------------------------------------------------------- */
/** \name Assert
 * \{ */

/* clang-format off */

#	define ROSE_assert(a) \
	(void)((!(a)) ? ((_ROSE_assert_backtrace(), _ROSE_assert_PRINT_POS(a), _ROSE_assert_ABORT(), NULL)) : NULL)

/* clang-format on */

/** \} */

/* -------------------------------------------------------------------- */
/** \name Assert With Message
 * \{ */

/* clang-format off */

#	define ROSE_assert_msg(a, msg) \
	(void)((!(a)) ? ((_ROSE_assert_backtrace(), _ROSE_assert_PRINT_POS(a), _ROSE_assert_print_extra(msg), _ROSE_assert_ABORT(), NULL)) : NULL)

/* clang-format on */

/** \} */

#else

/* -------------------------------------------------------------------- */
/** \name Assert
 * \{ */

#	define ROSE_assert(a) ((void)0)

/** \} */

/* -------------------------------------------------------------------- */
/** \name Assert With Message
 * \{ */

#	define ROSE_assert_msg(a, msg) ((void)0)

/** \} */

#endif

/* -------------------------------------------------------------------- */
/** \name Static Assert
 * \{ */

#if defined(__cplusplus)
#	define ROSE_STATIC_ASSERT(a, msg) static_assert(a, msg);
#elif defined(_MSC_VER)
#	if defined(__clang__)
#		define ROSE_STATIC_ASSERT(a, msg) _STATIC_ASSERT(a);
#	else
#		define ROSE_STATIC_ASSERT(a, msg) static_assert(a, msg);
#	endif
#else
#	define ROSE_STATIC_ASSERT(a, msg)
#endif

/** \} */

/* -------------------------------------------------------------------- */
/** \name Assert Unreachable Code
 * \{ */

/**
 * Indicates that this line of code should never be executed.
 * If it is reached, it will abort in debug builds and print an error in release builds.
 */
#define ROSE_assert_unreachable()                                             \
	{                                                                         \
		_ROSE_assert_unreachable_print(__FILE__, __LINE__, __func__);         \
		ROSE_assert_msg(0, "This line of code is marked to be unreachable."); \
	}                                                                         \
	((void)0)

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_ASSERT_H
