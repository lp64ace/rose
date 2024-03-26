#include <stdio.h>
#include <stdlib.h>

#include "LIB_assert.h"
#include "LIB_string_utf8.h"
#include "LIB_utildefines.h"

#include "LIB_string_cursor_utf8.h"

/**
 * The category of character as returned by #cursor_delim_type_unicode.
 *
 * \note Don't compare with any values besides #STRCUR_DELIM_NONE as cursor motion
 * should only delimit on changes, not treat some groups differently.
 *
 * For range calculation the order prioritizes expansion direction,
 * when the cursor is between two different categories, "hug" the smaller values.
 * Where white-space gets lowest priority. See #LIB_str_cursor_step_bounds_utf8.
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

static eStrCursorDelimType cursor_delim_type_unicode(const uint uch) {
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
		case 0xB4:	 /* Acute accent. */
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
		case 0xA3:			/* Pound sign. */
		case 0x80:			/* Euro sign. */
		case 0x3001:		/* CJK ideographic comma. */
		case 0xFF01:		/* CJK full width exclamation mark. */
		case 0xFF64:		/* CJK half width ideographic comma. */
		case 0xFF65:		/* Katakana half width middle dot. */
		case 0xFF1A:		/* CJK full width colon. */
		case 0xFF1B:		/* CJK full width semicolon. */
		case 0xFF1F:		/* CJK full width question mark. */
			/* case '_': */ /* special case, for python */
			return STRCUR_DELIM_OTHER;

		default:
			break;
	}
	return STRCUR_DELIM_ALPHANUMERIC; /* Not quite true, but ok for now */
}

static eStrCursorDelimType cursor_delim_type_utf8(const char *ch_utf8, const int ch_utf8_len, const int pos) {
	ROSE_assert(ch_utf8_len >= 0);
	/* for full unicode support we really need to have large lookup tables to figure
	 * out what's what in every possible char set - and python, glib both have these. */
	size_t index = (size_t)pos;
	uint uch = LIB_str_utf8_as_unicode_step_or_error(ch_utf8, (size_t)ch_utf8_len, &index);
	return cursor_delim_type_unicode(uch);
}

bool LIB_str_cursor_step_next_utf8(const char *str, const int str_maxlen, int *pos) {
	/* NOTE: Keep in sync with #LIB_str_cursor_step_next_utf32. */
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
	/* NOTE: Keep in sync with #LIB_str_cursor_step_prev_utf32. */
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
