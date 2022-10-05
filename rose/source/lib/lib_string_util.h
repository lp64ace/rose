#pragma once

#include <inttypes.h>
#include <stdarg.h>

#include "lib_compiler.h"
#include "lib_utildefines.h"

/**
 * Duplicates the first \a len bytes of cstring \a str
 * into a newly malloc'd string and returns it. \a str
 * is assumed to be at least len bytes long.
 *
 * \param str: The string to be duplicated
 * \param len: The number of bytes to duplicate
 * \retval Returns the duplicated string
 */
char *LIB_strdupn ( const char *str , size_t len ) ATTR_MALLOC ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

/**
 * Duplicates the cstring \a str into a newly malloc'd
 * string and returns it.
 *
 * \param str: The string to be duplicated
 * \retval Returns the duplicated string
 */
char *LIB_strdup ( const char *str ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( ) ATTR_MALLOC;

/**
 * Appends the two strings, and returns new malloc'ed string
 * \param str1: first string for copy
 * \param str2: second string for append
 * \retval Returns dst
 */
char *LIB_strdupcat ( const char *__restrict str1 , const char *__restrict str2 ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( ) ATTR_MALLOC;

/**
 * Like strncpy but ensures dst is always
 * '\0' terminated.
 *
 * \param dst: Destination for copy
 * \param src: Source string to copy
 * \param maxncpy: Maximum number of characters to copy (generally
 * the size of dst)
 * \retval Returns dst
 */
char *LIB_strncpy ( char *__restrict dst , const char *__restrict src , size_t maxncpy ) ATTR_NONNULL ( );

/**
 * Like strncpy but ensures dst is always
 * '\0' terminated.
 *
 * \note This is a duplicate of #BLI_strncpy that returns bytes copied.
 * And is a drop in replacement for 'snprintf(str, sizeof(str), "%s", arg);'
 *
 * \param dst: Destination for copy
 * \param src: Source string to copy
 * \param maxncpy: Maximum number of characters to copy (generally
 * the size of dst)
 * \retval The number of bytes copied (The only difference from BLI_strncpy).
 */
size_t LIB_strcpy_rlen ( char *__restrict dst , const char *__restrict src ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );

/**
 * Join an array of strings into a newly allocated, null terminated string.
 */
char *LIB_string_join_arrayN ( const char *strings [ ] , unsigned int strings_len ) ATTR_WARN_UNUSED_RESULT ATTR_NONNULL ( );
