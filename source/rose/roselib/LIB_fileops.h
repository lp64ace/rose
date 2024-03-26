#pragma once

#include "LIB_string.h"
#include "LIB_sys_types.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)
/* Path string comparisons: case-insensitive for Windows, case-sensitive otherwise. */
#	define LIB_path_cmp LIB_strcasecmp
#else
/* Path string comparisons: case-insensitive for Windows, case-sensitive otherwise. */
#	define LIB_path_cmp strcmp
#endif

/**
 * Returns the st_mode from stat-ing the specified path name, or 0 if stat fails
 * (most likely doesn't exist or no access).
 */
int LIB_exists(const char *path);

/**
 * opens the filename pointed to, by \a filepath using the given \a mode.
 */
FILE *LIB_fopen(const char *filepath, const char *mode);

/**
 * returns the current file position of the given \a stream.
 */
int64_t LIB_ftell(FILE *stream);

/**
 * sets the file position of the \a stream to the given \a offset.
 */
int LIB_fseek(FILE *stream, int64_t offset, int whence);

/**
 * Deletes the specified file or directory (depending on dir), optionally
 * doing recursive delete of directory contents.
 *
 * \return zero on success (matching 'remove' behavior).
 */
int LIB_delete(const char *file, bool dir, bool recursive);

#ifdef __cplusplus
}
#endif