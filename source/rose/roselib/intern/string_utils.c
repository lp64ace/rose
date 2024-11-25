#include "LIB_string.h"
#include "LIB_string_utils.h"

#include <wcwidth.h>

/* -------------------------------------------------------------------- */
/** \name Unicode Conversion
 * \{ */

size_t LIB_unicode_encode_as_utf8(char *out, size_t maxncpy, uint32_t unicode) {
	if (unicode <= 0x7f) {
		if (maxncpy >= 2) {
			out[0] = (char)unicode;
			out[1] = '\0';
		}
		else {
			out[0] = '\0';
		}
		return 1;
	}
	if (unicode <= 0x07ff) {
		if (maxncpy >= 3) {
			out[0] = (char)(((unicode >> 6) & 0x1F) | 0xC0);
			out[1] = (char)(((unicode >> 0) & 0x3F) | 0x80);
			out[2] = '\0';
		}
		else {
			out[0] = '\0';
		}
		return 2;
	}
	if (unicode <= 0xFFFF) {
		if (maxncpy >= 4) {
			out[0] = (char)(((unicode >> 12) & 0x0F) | 0xE0);
			out[1] = (char)(((unicode >> 6) & 0x3F) | 0x80);
			out[2] = (char)(((unicode >> 0) & 0x3F) | 0x80);
			out[3] = '\0';
		}
		else {
			out[0] = '\0';
		}
		return 3;
	}
	if (unicode <= 0x10FFFF) {
		if (maxncpy >= 5) {
			out[0] = (char)(((unicode >> 18) & 0x07) | 0xF0);
			out[1] = (char)(((unicode >> 12) & 0x3F) | 0x80);
			out[2] = (char)(((unicode >> 6) & 0x3F) | 0x80);
			out[3] = (char)(((unicode >> 0) & 0x3F) | 0x80);
			out[4] = '\0';
		}
		else {
			out[0] = '\0';
		}
		return 4;
	}
	if (maxncpy >= 4) {
		out[0] = (char)0xEF;
		out[1] = (char)0xBF;
		out[2] = (char)0xBD;
		out[3] = '\0';
	}
	else {
		out[0] = '\0';
	}
	return 0;
}

size_t LIB_strncpy_wchar_as_utf8(char *out, size_t maxncpy, const wchar_t *unicode) {
	ROSE_assert(maxncpy != 0);

	size_t len = 0;
	while (*unicode && len < maxncpy) {
		len += LIB_unicode_encode_as_utf8(out + len, maxncpy - len, (uint32_t)(*unicode++));
	}
	out[len] = '\0';
	/* Return the correct length when part of the final byte did not fit into the string. */
	while ((len > 0) && out[len - 1] == '\0') {
		len--;
	}
	return len;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name UTF8 Conversion
 * \{ */

ROSE_INLINE int utf8_char_compute_skip_or_error_with_mask(const unsigned char c, char *r_mask) {
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

ROSE_INLINE int utf8_char_compute_skip_or_error(const unsigned char c) {
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

ROSE_INLINE int utf8_char_compute_skip(const char c) {
	if (c >= 192) {
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

unsigned int LIB_str_utf8_as_unicode_step_or_error(const char *p, const size_t length, size_t *r_index) {
	const unsigned char c = (unsigned char)(*(p += *r_index));

	ROSE_assert(*r_index < length);
	ROSE_assert(c != '\0');

	char mask = 0;
	const int len = utf8_char_compute_skip_or_error_with_mask(c, &mask);
	if (len == -1 || (*r_index + len > length)) {
		return ROSE_UTF8_ERR;
	}

	const uint result = utf8_char_decode(p, mask, len, ROSE_UTF8_ERR);
	if (result == ROSE_UTF8_ERR) {
		return ROSE_UTF8_ERR;
	}
	*r_index += len;
	ROSE_assert(*r_index <= length);
	return result;
}

int LIB_wcwidth_or_error(wchar_t ucs) {
	/* Treat private use areas (icon fonts), symbols, and emoticons as double-width. */
	if (ucs >= 0xf0000 || (ucs >= 0xe000 && ucs < 0xf8ff) || (ucs >= 0x1f300 && ucs < 0x1fbff)) {
		return 2;
	}
	return mk_wcwidth(ucs);
}

int LIB_wcwidth_safe(wchar_t ucs) {
	const int columns = LIB_wcwidth_or_error(ucs);
	if (columns >= 0) {
		return columns;
	}
	return 1;
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

bool LIB_str_cursor_step_next_utf8(const char *str, const int str_maxlen, int *pos) {
	ROSE_assert(str_maxlen >= 0);
	ROSE_assert(*pos >= 0);

	if (*pos >= str_maxlen) {
		return false;
	}
	const char *str_end = str + (str_maxlen + 1);
	const char *str_pos = str + *pos;
	const char *str_next = str_pos;
	do {
		str_next = LIB_str_find_next_char_utf8(str_next, str_end);
	} while ((str_next < str_end) && (str_next[0] != 0) && (LIB_str_utf8_char_width_or_error(str_next) == 0));
	*pos += (int)(str_next - str_pos);
	if (*pos > str_maxlen) {
		*pos = str_maxlen;
	}

	return true;
}

bool LIB_str_cursor_step_prev_utf8(const char *str, const int str_maxlen, int *pos) {
	ROSE_assert(str_maxlen >= 0);
	ROSE_assert(*pos >= 0);

	if ((*pos > 0) && (*pos <= str_maxlen)) {
		const char *str_pos = str + *pos;
		const char *str_prev = str_pos;
		do {
			str_prev = LIB_str_find_prev_char_utf8(str_prev, str);
		} while ((str_prev > str) && (LIB_str_utf8_char_width_or_error(str_prev) == 0));
		*pos -= (int)(str_pos - str_prev);
		return true;
	}

	return false;
}

/**
 * The category of character as returned by #cursor_delim_type_unicode.
 *
 * \note Don't compare with any values besides #STRCUR_DELIM_NONE as cursor motion
 * should only delimit on changes, not treat some groups differently.
 *
 * For range calculation the order prioritizes expansion direction,
 * when the cursor is between two different categories, "hug" the smaller values.
 * Where white-space gets lowest priority. See #BLI_str_cursor_step_bounds_utf8.
 * This is done so expanding the range at a word boundary always chooses the word instead
 * of the white-space before or after it.
 */
typedef enum eStrCursorDelimType {
	STRCUR_DELIM_NONE,
	STRCUR_DELIM_ALPHANUMERIC,
	STRCUR_DELIM_PUNCT,
	STRCUR_DELIM_BRACE,
	STRCUR_DELIM_OPERATOR,
	STRCUR_DELIM_QUOTE,
	STRCUR_DELIM_OTHER,
	STRCUR_DELIM_WHITESPACE,
} eStrCursorDelimType;

static eStrCursorDelimType cursor_delim_type_unicode(const unsigned int uch) {
	switch (uch) {
		case ',':
		case '.':
		case 0x2026: /* Horizontal ellipsis. */
		case 0x3002: /* CJK full width full stop. */
		case 0xFF0C: /* CJK full width comma. */
		case 0xFF61: /* CJK half width full stop. */
			return STRCUR_DELIM_PUNCT;
	
		case '{':
		case '}':
		case '[':
		case ']':
		case '(':
		case ')':
		case 0x3010: /* CJK full width left black lenticular bracket. */
		case 0x3011: /* CJK full width right black lenticular bracket. */
		case 0xFF08: /* CJK full width left parenthesis. */
		case 0xFF09: /* CJK full width right parenthesis. */
			return STRCUR_DELIM_BRACE;
	
		case '+':
		case '-':
		case '=':
		case '~':
		case '%':
		case '/':
		case '<':
		case '>':
		case '^':
		case '*':
		case '&':
		case '|':
		case 0x2014: /* Em dash. */
		case 0x300A: /* CJK full width left double angle bracket. */
		case 0x300B: /* CJK full width right double angle bracket. */
		case 0xFF0F: /* CJK full width solidus (forward slash). */
		case 0xFF5E: /* CJK full width tilde. */
			return STRCUR_DELIM_OPERATOR;
	
		case '\'':
		case '\"':
		case '`':
		case 0xB4:   /* Acute accent. */
		case 0x2018: /* Left single quotation mark. */
		case 0x2019: /* Right single quotation mark. */
		case 0x201C: /* Left double quotation mark. */
		case 0x201D: /* Right double quotation mark. */
			return STRCUR_DELIM_QUOTE;
	
		case ' ':
		case '\t':
		case '\n':
			return STRCUR_DELIM_WHITESPACE;
	
		case '\\':
		case '@':
		case '#':
		case '$':
		case ':':
		case ';':
		case '?':
		case '!':
		case 0xA3:        /* Pound sign. */
		case 0x80:        /* Euro sign. */
		case 0x3001:      /* CJK ideographic comma. */
		case 0xFF01:      /* CJK full width exclamation mark. */
		case 0xFF64:      /* CJK half width ideographic comma. */
		case 0xFF65:      /* Katakana half width middle dot. */
		case 0xFF1A:      /* CJK full width colon. */
		case 0xFF1B:      /* CJK full width semicolon. */
		case 0xFF1F:      /* CJK full width question mark. */
			/* case '_': */ /* special case, for python */
			return STRCUR_DELIM_OTHER;
	
		default:
			break;
	}
	return STRCUR_DELIM_ALPHANUMERIC; /* Not quite true, but ok for now */
}

ROSE_STATIC eStrCursorDelimType cursor_delim_type_utf8(const char *ch_utf8, const int ch_utf8_len, const int pos) {
	ROSE_assert(ch_utf8_len >= 0);
	size_t index = (size_t)pos;
	unsigned int uch = LIB_str_utf8_as_unicode_step_or_error(ch_utf8, (size_t)ch_utf8_len, &index);
	return cursor_delim_type_unicode(uch);
}

void LIB_str_cursor_step_bounds_utf8(const char *p, int n, int pos, int *l, int *r) {
	ROSE_assert(n >= 0);
	ROSE_assert(pos >= 0 && pos <= n);
	
	eStrCursorDelimType prev = (pos > 0) ? cursor_delim_type_utf8(p, n, pos - 1) : STRCUR_DELIM_NONE;
	eStrCursorDelimType next = (pos < n) ? cursor_delim_type_utf8(p, n, pos + 0) : STRCUR_DELIM_NONE;
	
	*l = pos;
	*r = pos;
	
	if(prev != STRCUR_DELIM_NONE) {
		while (*l > 0 && ((prev <= next) || (next == STRCUR_DELIM_NONE))) {
			LIB_str_cursor_step_prev_utf8(p, n, l);
			if (*l > 0) {
				prev = cursor_delim_type_utf8(p, n, *l - 1);
			}
		}
	}
	if(next != STRCUR_DELIM_NONE) {
		while (*r < n && ((next <= prev) || (prev == STRCUR_DELIM_NONE))) {
			LIB_str_cursor_step_next_utf8(p, n, r);
			if (*r < n) {
				next = cursor_delim_type_utf8(p, n, *r);
			}
		}
	}
}

unsigned int LIB_str_utf8_as_unicode_or_error(const char *p) {
	const unsigned char c = *p;

	char mask = 0;
	const int len = utf8_char_compute_skip_or_error_with_mask(c, &mask);
	if (len == -1) {
		return ROSE_UTF8_ERR;
	}
	return utf8_char_decode(p, mask, len, ROSE_UTF8_ERR);
}

unsigned int LIB_str_utf8_char_width_or_error(const char *p) {
	uint unicode = LIB_str_utf8_as_unicode_or_error(p);
	if (unicode == ROSE_UTF8_ERR) {
		return -1;
	}

	return LIB_wcwidth_or_error(unicode);
}

unsigned int LIB_str_utf8_size_or_error(const char *p) {
	return utf8_char_compute_skip_or_error(*(const unsigned char *)p);
}

unsigned int LIB_str_utf8_size_safe(const char *p) {
	return utf8_char_compute_skip(*(const unsigned char *)p);
}

unsigned int LIB_str_utf8_as_unicode_step_safe(const char *p, size_t length, size_t *r_index) {
	uint result = LIB_str_utf8_as_unicode_step_or_error(p, length, r_index);
	if (result == ROSE_UTF8_ERR) {
		result = (unsigned int)(p[*r_index]);
		*r_index += 1;
	}
	ROSE_assert(*r_index <= length);
	return result;
}

/** \} */
