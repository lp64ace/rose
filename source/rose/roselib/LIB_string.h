#pragma once

#include <stdarg.h>

#include "LIB_compiler_attrs.h"

#define LIB_NPOS ((size_t)-1)

/* -------------------------------------------------------------------- */
/** \name String Utils
 * \{ */

/**
 * Returns a pointer to a null-terminated byte string, which is a duplicate of the string pointed to by #text. The returned
 * pointer must be passed to MEM_freeN to avoid a memory leak.
 */
char *LIB_strdupN(const char *text);

/**
 * Returns a pointer to a null-terminated byte string, which contains copies of at most #length bytes from the string pointed
 * to by #text. If the null terminator is not encountered in the first #length bytes, it is added to the duplicated string.
 */
char *LIB_strndupN(const char *text, const size_t length);

/* \} */

/* -------------------------------------------------------------------- */
/** \name String Queries
 * \{ */

/**
 * The length of a C string is determined by the terminating null-character: A C string is as long as the number of characters
 * between the beginning of the string and the terminating null character (without including the terminating null character
 * itself).
 */
size_t LIB_strlen(const char *text);

/**
 * The length of a C string is determined by the terminating null-character: A C string is as long as the number of characters
 * between the beginning of the string and the terminating null character (without including the terminating null character
 * itself).
 */
size_t LIB_strnlen(const char *text, const size_t maxlen);

/* \} */

/* -------------------------------------------------------------------- */
/** \name String Search
 * \{ */

/**
 * This function searches for the specified #needle string within the #haystack string and returns the position (index) of the
 * first occurence. If the #needle is not found, the return value is #LIB_NPOS, which indices the absence of the substring.
 */
size_t LIB_strfind(const char *haystack, const char *needle);

/* \} */

/* -------------------------------------------------------------------- */
/** \name String Format
 * \{ */
 
/**
 * A function that is extremely similar to vsnprintf but we can assume that if the return value is not LIB_NPOS the string has
 * been completely written in the #r_buffer and is also NULL terminated.
 * \param r_buffer The buffer we want to write the resulting string into.
 * \param buflen Usually the length of the buffer.
 * \param fmt The printf format string.
 * \param ... The arguments for the format string.
 */
size_t LIB_vsnprintf(char *r_buffer, size_t buflen, ATTR_PRINTF_FORMAT const char *fmt, va_list arg_ptr);
 
/**
 * A function that is extremely similar to snprintf but we can assume that if the return value is not LIB_NPOS the string has
 * been completely written in the #r_buffer and is also NULL terminated.
 * \param r_buffer The buffer we want to write the resulting string into.
 * \param buflen Usually the length of the buffer.
 * \param fmt The printf format string.
 * \param ... The arguments for the format string.
 */
size_t LIB_snprintf(char *r_buffer, size_t buflen, ATTR_PRINTF_FORMAT const char *fmt, ...);

/**
 * Like #LIB_snprintf this function will format the string according to the printf standard and store the result in a newly
 * allocated string.
 * \param fmt The printf format string.
 * \param ... The arguments for the format string.
 */
char *LIB_sprintf_allocN(ATTR_PRINTF_FORMAT const char *fmt, ...);
 
/* \} */

/* -------------------------------------------------------------------- */
/** \name String Editing
 * \{ */
 
/** This will replace any of occurances of `src` in `str` with `dst`. */
size_t LIB_str_replace_char(char *str, const char src, const char dst);

/** This will replace any of occurances of `src` in `str` with `dst`. */
size_t LIB_str_replacen_char(char *str, size_t maxlen, const char src, const char dst);

/**
 * Replace any occurances of `needle` inside of `haystack` with `alt` and store the output in `buffer`.
 * \return Returns the length of the `buffer` in bytes not including the null-terminator or LIB_NPOS if an error occured.
 */
size_t LIB_str_replace(char *buffer, size_t maxlen, const char *haystack, const char *needle, const char *alt);
 
/* \} */
