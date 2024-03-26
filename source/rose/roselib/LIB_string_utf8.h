#pragma once

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LIB_UTF8_ERR ((unsigned int)-1)

/**
 * \param p: a pointer to Unicode character encoded as UTF-8
 *
 * Converts a sequence of bytes encoded as UTF-8 to a Unicode character.
 * If \a p does not point to a valid UTF-8 encoded character, results are
 * undefined. If you are not sure that the bytes are complete
 * valid Unicode characters, you should use g_utf8_get_char_validated()
 * instead.
 *
 * Return value: the resulting character
 */
unsigned int LIB_str_utf8_as_unicode_or_error(const char *p);

int LIB_str_utf8_char_width_or_error(const char *p);
int LIB_str_utf8_char_width_safe(const char *p);

/**
 * Count columns that character/string occupies (based on `wcwidth.co`).
 */
int LIB_wcwidth_or_error(char32_t ucs);
int LIB_wcwidth_safe(char32_t ucs);
int LIB_wcswidth_or_error(const char32_t *pwcs, size_t n);

/**
 * \param p: pointer to some position within \a str
 * \param str_start: pointer to the beginning of a UTF-8 encoded string
 *
 * Given a position \a p with a UTF-8 encoded string \a str, find the start
 * of the previous UTF-8 character starting before. \a p Returns \a str_start if no
 * UTF-8 characters are present in \a str_start before \a p.
 *
 * \a p does not have to be at the beginning of a UTF-8 character. No check
 * is made to see if the character found is actually valid other than
 * it starts with an appropriate byte.
 *
 * \return A pointer to the found character.
 */
const char *LIB_str_find_prev_char_utf8(const char *p, const char *str_start);
/**
 * \param p: a pointer to a position within a UTF-8 encoded string
 * \param str_end: a pointer to the byte following the end of the string.
 *
 * Finds the start of the next UTF-8 character in the string after \a p
 *
 * \a p does not have to be at the beginning of a UTF-8 character. No check
 * is made to see if the character found is actually valid other than
 * it starts with an appropriate byte.
 *
 * \return a pointer to the found character or a pointer to the null terminating character '\0'.
 */
const char *LIB_str_find_next_char_utf8(const char *p, const char *str_end);

unsigned int LIB_str_utf8_as_unicode_or_error(const char *p);
unsigned int LIB_str_utf8_as_unicode_safe(const char *p);
/**
 * UTF8 decoding that steps over the index.
 * When an error is encountered fall back to `LATIN1`, stepping over a single byte.
 *
 * \param p: The text to step over.
 * \param p_len: The length of `p`.
 * \param index: Index of `p` to step over.
 * \return the code-point `(p + *index)` if there is a decoding error.
 */
unsigned int LIB_str_utf8_as_unicode_step_safe(const char *__restrict p, size_t p_len, size_t *__restrict index);

#ifdef __cplusplus
}
#endif
