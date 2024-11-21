#ifndef LIB_STRING_H
#define LIB_STRING_H

#include "LIB_utildefines.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name String Utils
 * \{ */

/**
 * Returns a pointer to a null-terminated byte string,
 * which is a duplicate of the string pointed to by \p text.
 *
 * \note The returned pointer must be passed to MEM_freeN to avoid a memory leak.
 */
char *LIB_strdupN(const char *text);

/**
 * Returns a pointer to a null-terminated byte string,
 * which contains copies of at most \p length bytes from the string pointed to by \p text.
 *
 * If the null terminator is not encountered in the first \p length bytes,
 * it is added to the duplicated string.
 *
 * \note The returned pointer must be passed to MEM_freeN to avoid a memory leak.
 */
char *LIB_strndupN(const char *text, const size_t length);

/**
 * Copies a source string to a destination string and ensures that it is null terminated.
 *
 * \param dst The destination pointer to write the copied string.
 * \param dst_maxncpy The maxinum number of charachter to write to the destination buffer.
 * \param src The source string we want to copy.
 *
 * \return Returns \p dst.
 */
char *LIB_strcpy(char *dst, size_t dst_maxncpy, const char *src);
size_t LIB_strcpy_rlen(char *dst, size_t dst_maxncpy, const char *src);
/**
 * Copies a source string to a destination string and ensures that it is null terminated.
 *
 * \param dst The destination pointer to write the copied string.
 * \param dst_maxncpy The maxinum number of charachter to write to the destination buffer.
 * \param src The source string we want to copy.
 * \param length The length of the source string we want to copy.
 *
 * \return Returns \p dst.
 */
char *LIB_strncpy(char *dst, size_t dst_maxncpy, const char *src, size_t length);

/**
 * \brief Concatenates the source string to the destination string.
 *
 * This function appends the \p src string to the end of the \p dst string.
 * The \p dst_maxncpy parameter specifies the maximum number of characters
 * to be copied to \p dst, including the null terminator.
 *
 * \param dst The destination string to which the source string will be appended.
 *            The destination string must have enough space to accommodate
 *            the concatenated result, including the null terminator.
 * \param dst_maxncpy The maximum number of characters to copy to \p dst,
 *                    including the null terminator.
 * \param src The source string to be appended to the destination string.
 *
 * \return A pointer to the destination string \p dst.
 *
 * \note If the length of \p dst plus the length of \p src exceeds \p dst_maxncpy - 1,
 *       the resulting \p dst string will be truncated, and it will always be null-terminated.
 *       If \p dst_maxncpy is zero, the function returns \p dst without any modification.
 */
char *LIB_strcat(char *dst, size_t dst_maxncpy, const char *src);
/**
 * \brief Appends a portion of the source string to the destination string.
 *
 * This function appends up to \p length characters from the \p src string
 * to the end of the \p dst string. The \p dst_maxncpy parameter specifies
 * the maximum number of characters to be copied to \p dst, including the
 * null terminator.
 *
 * \param dst The destination string to which the source string will be appended.
 *            The destination string must have enough space to accommodate
 *            the concatenated result, including the null terminator.
 * \param dst_maxncpy The maximum number of characters to copy to \p dst,
 *                    including the null terminator.
 * \param src The source string from which characters will be appended to the destination string.
 * \param length The maximum number of characters to append from the \p src string.
 *
 * \return A pointer to the destination string \p dst.
 *
 * \note If the length of \p dst plus the smaller of \p length or the actual length of \p src
 *       exceeds \p dst_maxncpy - 1, the resulting \p dst string will be truncated, and it will
 *       always be null-terminated. If \p dst_maxncpy is zero, the function returns \p dst
 *       without any modification.
 */
char *LIB_strncat(char *dst, size_t dst_maxncpy, const char *src, size_t length);

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Queries
 * \{ */

/**
 * The length of a C string is determined by the terminating null-character:
 * A C string is as long as the number of characters between the beginning of the string and
 * the terminating null character (without including the terminating null character
 * itself).
 */
size_t LIB_strlen(const char *text);

/**
 * The length of a C string is determined by the terminating null-character:
 * A C string is as long as the number of characters between the beginning of the string and
 * the terminating null character (without including the terminating null character itself).
 */
size_t LIB_strnlen(const char *text, const size_t maxlen);

/**
 * \brief Checks if the given string starts with the specified prefix.
 *
 * This function compares the start of the input string `text` with the
 * string `prefix` to determine if `text` begins with `prefix`.
 *
 * \param text The main string to check.
 * \param prefix The prefix to look for at the beginning of the string.
 * \return true if \p text starts with \p prefix, false otherwise.
 */
bool LIB_str_starts_with(const char *text, const char *prefix);

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Search
 * \{ */

/**
 * \brief Finds the first occurrence of a word within a specified range in a string.
 *
 * This function searches for the first occurrence of the substring \p word
 * within the range specified by the pointers \p begin and \p end. The search
 * is case-sensitive.
 *
 * \param begin A pointer to the beginning of the range where the search should start.
 * \param end A pointer to one past the last character in the range where the search should end.
 * \param word A pointer to the substring to search for within the specified range.
 *
 * \return A pointer to the first occurrence of \p word within the specified range.
 *         If \p word is not found, the function returns `NULL`.
 */
const char *LIB_strfind(const char *begin, const char *end, const char *word);

/**
 * \brief Finds the last occurrence of a word within a specified range in a string.
 *
 * This function searches for the last occurrence of the substring \p word
 * within the range specified by the pointers \p begin and \p end. The search
 * is case-sensitive.
 *
 * \param begin A pointer to the beginning of the range where the search should start.
 * \param end A pointer to one past the last character in the range where the search should end.
 * \param word A pointer to the substring to search for within the specified range.
 *
 * \return A pointer to the last occurrence of \p word within the specified range.
 *         If \p word is not found, the function returns `NULL`.
 */
const char *LIB_strrfind(const char *begin, const char *end, const char *word);

/**
 * \brief Finds the next occurrence of a character in a string starting from a specific iterator.
 *
 * This function searches for the next occurrence of a specified character \p c
 * within a given range in a string, starting from a specific iterator position.
 *
 * \param begin Pointer to the start of the string to search.
 * \param end Pointer to the end of the string to search (one past the last character).
 * \param itr Pointer to the current position in the string to start the search.
 * \param c The character to search for.
 * \return A pointer to the next occurrence of the character \p c in the string
 *         after the \p itr position, or \p end if the character is not found within
 *         the specified range.
 *
 * \note If \p end is before \p begin, or \p itr is not within the range [\p begin, \p end),
 *       the behavior is undefined. If \p c is not found, the function returns \p end.
 */
const char *LIB_strnext(const char *begin, const char *end, const char *itr, int c);

/**
 * \brief Finds the previous occurrence of a character in a string starting from a specific iterator.
 *
 * This function searches for the previous occurrence of a specified character \p c
 * within a given range in a string, starting from a specific iterator position.
 *
 * \param begin Pointer to the start of the string to search.
 * \param end Pointer to the end of the string to search (one past the last character).
 * \param itr Pointer to the current position in the string to start the search.
 * \param c The character to search for.
 * \return A pointer to the previous occurrence of the character \p c in the string
 *         before the \p itr position, or \p begin - 1 if the character is not found
 *         within the specified range.
 *
 * \note If \p end is before \p begin, or \p itr is not within the range [\p begin, \p end),
 *       the behavior is undefined. If \p c is not found, the function returns \p end.
 */
const char *LIB_strprev(const char *begin, const char *end, const char *itr, int c);

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Format
 * \{ */

/**
 * \brief Formats a string and stores the result in the provided buffer with size constraints.
 *
 * This function behaves similarly to `snprintf`, but with custom functionality for formatting
 * and storing the formatted string into the buffer. It ensures that the resulting string
 * does not exceed the specified buffer size, including the null terminator.
 *
 * \param buffer The destination buffer where the formatted string will be stored.
 * \param maxncpy The maximum number of characters to copy, including the null terminator.
 * \param fmt The format string that specifies how subsequent arguments are converted for output.
 * This should follow standard `printf` formatting rules.
 * \param ... Additional arguments as specified in the format string.
 *
 * \return Returns the number of bytes written into the buffer if successful. If the buffer is
 * not long enough or there is an error, the return value is 0.
 *
 * \note The `buffer` will always be null-terminated, and no truncation will occur.
 */
size_t LIB_strnformat(char *buffer, size_t maxncpy, ATTR_PRINTF_FORMAT const char *fmt, ...);
size_t LIB_vstrnformat(char *buffer, size_t maxncpy, ATTR_PRINTF_FORMAT const char *fmt, va_list args);

char *LIB_strformat_allocN(ATTR_PRINTF_FORMAT const char *fmt, ...);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Replace in String
 * \{ */

/** Replace all occurances of #old in #buffer with #nval. */
void LIB_string_replace_single(char *buffer, char old, char nval);

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Build
 * \{ */

/**
 * Join an array of strings into a newly allocated, null terminated string.
 */
char *LIB_string_join_arrayN(const char **array, size_t length);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_STRING_H
