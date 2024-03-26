#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_string_utf8.h"

#include <wcwidth.h>

/* -------------------------------------------------------------------- */
/** \name UTF8 Character Decoding (Skip & Mask Lookup)
 *
 * Derived from GLIB `gutf8.c`.
 *
 * Ranges (zero based, inclusive):
 *
 * - 000..127: 1 byte.
 * - 128..191: invalid.
 * - 192..223: 2 bytes.
 * - 224..239: 3 bytes.
 * - 240..247: 4 bytes.
 * - 248..251: 4 bytes.
 * - 252..253: 4 bytes.
 * - 254..255: invalid.
 *
 * Invalid values fall back to 1 byte or -1 (for an error value).
 *
 * \note From testing string copying via #BLI_strncpy_utf8 with large (multi-megabyte) strings,
 * using a function instead of a lookup-table is between 2 & 3 times faster.
 * \{ */

ROSE_INLINE int utf8_char_compute_skip(const char c) {
	if (UNLIKELY(c >= 192)) {
		if ((c & 0xe0) == 0xc0) {
			return 2;
		}
		if ((c & 0xf0) == 0xe0) {
			return 3;
		}
		if ((c & 0xf8) == 0xf0) {
			return 4;
		}
		if ((c & 0xfc) == 0xf8) {
			return 5;
		}
		if ((c & 0xfe) == 0xfc) {
			return 6;
		}
	}
	return 1;
}

ROSE_INLINE int utf8_char_compute_skip_or_error(const char c) {
	if (c < 128) {
		return 1;
	}
	if ((c & 0xe0) == 0xc0) {
		return 2;
	}
	if ((c & 0xf0) == 0xe0) {
		return 3;
	}
	if ((c & 0xf8) == 0xf0) {
		return 4;
	}
	if ((c & 0xfc) == 0xf8) {
		return 5;
	}
	if ((c & 0xfe) == 0xfc) {
		return 6;
	}
	return -1;
}

ROSE_INLINE int utf8_char_compute_skip_or_error_with_mask(const char c, char *r_mask) {
	/* Originally from GLIB `UTF8_COMPUTE` macro. */
	if (c < 128) {
		*r_mask = 0x7f;
		return 1;
	}
	if ((c & 0xe0) == 0xc0) {
		*r_mask = 0x1f;
		return 2;
	}
	if ((c & 0xf0) == 0xe0) {
		*r_mask = 0x0f;
		return 3;
	}
	if ((c & 0xf8) == 0xf0) {
		*r_mask = 0x07;
		return 4;
	}
	if ((c & 0xfc) == 0xf8) {
		*r_mask = 0x03;
		return 5;
	}
	if ((c & 0xfe) == 0xfc) {
		*r_mask = 0x01;
		return 6;
	}
	return -1;
}

/**
 * Decode a UTF8 code-point, use in combination with #utf8_char_compute_skip_or_error_with_mask.
 */
ROSE_INLINE uint utf8_char_decode(const char *p, const char mask, const int len, const uint err) {
	/* Originally from GLIB `UTF8_GET` macro, added an 'err' argument. */
	uint result = p[0] & mask;
	for (int count = 1; count < len; count++) {
		if ((p[count] & 0xc0) != 0x80) {
			return err;
		}
		result <<= 6;
		result |= p[count] & 0x3f;
	}
	return result;
}

/** \} */

int LIB_wcwidth_or_error(char32_t ucs) {
	/* Treat private use areas (icon fonts), symbols, and emoticons as double-width. */
	if (ucs >= 0xf0000 || (ucs >= 0xe000 && ucs < 0xf8ff) || (ucs >= 0x1f300 && ucs < 0x1fbff)) {
		return 2;
	}
	return mk_wcwidth(ucs);
}

int LIB_wcwidth_safe(char32_t ucs) {
	const int columns = LIB_wcwidth_or_error(ucs);
	if (columns >= 0) {
		return columns;
	}
	return 1;
}

int LIB_wcswidth_or_error(const char32_t *pwcs, size_t n) {
	return mk_wcswidth(pwcs, n);
}

uint LIB_str_utf8_as_unicode_or_error(const char *p) {
	const uchar c = (uchar)(*p);

	char mask = 0;
	const int len = utf8_char_compute_skip_or_error_with_mask(c, &mask);
	if (UNLIKELY(len == -1)) {
		return LIB_UTF8_ERR;
	}
	return utf8_char_decode(p, mask, len, LIB_UTF8_ERR);
}

int LIB_str_utf8_char_width_or_error(const char *p) {
	uint unicode = LIB_str_utf8_as_unicode_or_error(p);
	if (unicode == LIB_UTF8_ERR) {
		return -1;
	}

	return LIB_wcwidth_or_error((char32_t)unicode);
}

int LIB_str_utf8_char_width_safe(const char *p) {
	uint unicode = LIB_str_utf8_as_unicode_or_error(p);
	if (unicode == LIB_UTF8_ERR) {
		return 1;
	}

	return LIB_wcwidth_safe((char32_t)unicode);
}

const char *LIB_str_find_prev_char_utf8(const char *p, const char *str_start) {
	ROSE_assert(p >= str_start);
	if (str_start < p) {
		for (--p; p >= str_start; p--) {
			if ((*p & 0xc0) != 0x80) {
				return (char *)p;
			}
		}
	}
	return p;
}

const char *LIB_str_find_next_char_utf8(const char *p, const char *str_end) {
	ROSE_assert(p <= str_end);
	if ((p < str_end) && (*p != '\0')) {
		for (++p; p < str_end && (*p & 0xc0) == 0x80; p++) {
			/* do nothing */
		}
	}
	return p;
}

uint LIB_str_utf8_as_unicode_step_or_error(const char *__restrict p, const size_t p_len, size_t *__restrict index) {
	const uchar c = (uchar)(*(p += *index));

	ROSE_assert(*index < p_len);
	ROSE_assert(c != '\0');

	char mask = 0;
	const int len = utf8_char_compute_skip_or_error_with_mask(c, &mask);
	if (UNLIKELY(len == -1) || (*index + (size_t)(len) > p_len)) {
		return LIB_UTF8_ERR;
	}

	const uint result = utf8_char_decode(p, mask, len, LIB_UTF8_ERR);
	if (UNLIKELY(result == LIB_UTF8_ERR)) {
		return LIB_UTF8_ERR;
	}
	*index += (size_t)(len);
	ROSE_assert(*index <= p_len);
	return result;
}

unsigned int LIB_str_utf8_as_unicode_step_safe(const char *__restrict p, size_t p_len, size_t *__restrict index) {
	uint result = LIB_str_utf8_as_unicode_step_or_error(p, p_len, index);
	if (UNLIKELY(result == LIB_UTF8_ERR)) {
		result = (uint)p[*index];
		*index += 1;
	}
	ROSE_assert(*index <= p_len);
	return result;
}
