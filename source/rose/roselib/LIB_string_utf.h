#ifndef LIB_STRING_UTF_H
#define LIB_STRING_UTF_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ROSE_UTF8_MAX 6
#define ROSE_UTF8_WIDTH_MAX 2 /* columns */
#define ROSE_UTF8_ERR ((unsigned int)-1)

/* -------------------------------------------------------------------- */
/** \name Unicode
 * \{ */

/** Encode a unicode code point using UTF-8. */
size_t LIB_unicode_encode_as_utf8(char *out, size_t maxncpy, uint32_t unicode);
/** Encode a unicode string using UTF-8. */
size_t LIB_strncpy_wchar_as_utf8(char *out, size_t maxncpy, const wchar_t *unicode);

int LIB_wcwidth_or_error(wchar_t ucs);
int LIB_wcwidth_safe(wchar_t ucs);

/** \} */

/* -------------------------------------------------------------------- */
/** \name UTF8
 * \{ */

unsigned int LIB_str_utf8_size_safe(const char *p);

const char *LIB_str_find_next_char_utf8(const char *p, const char *str_end);
const char *LIB_str_find_prev_char_utf8(const char *p, const char *str_start);

bool LIB_str_cursor_step_next_utf8(const char *str, int str_maxlen, int *pos);
bool LIB_str_cursor_step_prev_utf8(const char *str, int str_maxlen, int *pos);

void LIB_str_cursor_step_bounds_utf8(const char *p, int maxlen, int pos, int *left, int *right);

unsigned int LIB_str_utf8_char_width_or_error(const char *p);
unsigned int LIB_str_utf8_size_or_error(const char *p);

unsigned int LIB_str_utf8_as_unicode_step_safe(const char *p, size_t length, size_t *r_index);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_STRING_UTF_H
