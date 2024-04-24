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

/** Opens for reading. If the file doesn't exist or can't be found, the fopen call fails. */
#define LIB_FMODE_READ "r"
/** Opens an empty file for writing. If the given file exists, its contents are destroyed. */
#define LIB_FMODE_WRITE "w"
/**
 * Opens for writing at the end of the file (appending) without removing the end-of-file (EOF) marker before new data is written to the file. 
 * Creates the file if it doesn't exist.
 */
#define LIB_FMODE_APPEND "a"
/** Opens for both reading and writing. The file must exist. */
#define LIB_FMODE_READWRITE "r+"
/** Opens an empty file for both reading and writing. If the file exists, its contents are destroyed. */
#define LIB_FMODE_WRITEREAD "w+"
/**
 * Opens for reading and appending. 
 * The appending operation includes the removal of the EOF marker before new data is written to the file. 
 * The EOF marker isn't restored after writing is completed. 
 * Creates the file if it doesn't exist.
 */
#define LIB_FMODE_READAPPEND "a+"

#if defined(WIN32)
#	define LIB_XMODE_BINARY ""
#else
#	define LIB_XMODE_BINARY "b"
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
 * close the file associated with the given \a stream.
 */
void LIB_fclose(FILE *stream);

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