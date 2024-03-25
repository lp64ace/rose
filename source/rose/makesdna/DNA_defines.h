#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Copied from #LIB_endian.h
 *
 * We don't want to include any extra header files here since we want to
 * be able to compile this module seperately with clang.
 * \{ */

#define __ENDIAN_WORD__ 'Rose'

#if __ENDIAN_WORD__ == 0x526F7365
#	define BIG_ENDIAN
#elif __ENDIAN_WORD__ == 0x65736F52
#	define LITTLE_ENDIAN
#endif

/* \} */

#define MAX_NAME 64
#define MAX_PATH 1024

enum {
	/** This shouldn't really be used but maybe we can find a use for it later on. */
	DNA_ACTION_NONE = 0,
	/** This indicates that the typename should only be stored in file if referenced by on that is. */
	DNA_ACTION_RUNTIME = 1,
	/** This indicates that the typename should be stored in file. */
	DNA_ACTION_STORE = 2,
};

#ifdef DNA_CLANG
#define DNA_CLANG_ENABLE_FUNCTION_NAME(typename, action) clang_enable_ ## typename
#define DNA_ACTION_DEFINE(typename, action) \
	void DNA_CLANG_ENABLE_FUNCTION_NAME(typename, action)(struct typename *ptr) { \
		memset(ptr, 0, sizeof(*ptr)); \
	}
#else
#define DNA_ACTION_DEFINE(typename, action)
#endif

#ifdef __cplusplus
}
#endif
