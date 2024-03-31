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

#ifdef __cplusplus
}
#endif
