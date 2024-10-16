#include "LIB_string.h"

#include "ast/type.h"

#include "RT_context.h"
#include "RT_source.h"
#include "RT_token.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

ROSE_INLINE bool token_strhas(RCCToken *t, const char *str) {
	if (LIB_strfind(t->location.p, t->location.p + t->length, str)) {
		return true;
	}
	return false;
}

ROSE_INLINE bool token_chrhas(RCCToken *t, char c) {
	for (const char *itr = t->location.p; itr != t->location.p + t->length; itr++) {
		if (*itr == c) {
			return true;
		}
	}
	return false;
}

ROSE_INLINE int to_hexadecimal(int c) {
	return ('0' <= c && c <= '9') ? c - '0' : tolower(c) - 'a' + 10;
}

ROSE_INLINE int translate_escaped_sequence(const char **rest, const char *p) {
	if ('0' <= p[0] && p[0] <= '7') {
		int ret = 0;
		for (; ret < 64 && ('0' <= *p && *p <= '7'); *rest = ++p) {
			ret = ret * 8 + ((*rest)[0] - '0');
		}
		return ret;
	}
	if (ELEM(p[0], 'x', 'X')) {
		int ret = 0;
		for (; ('0' <= *p && *p <= '9') && ('a' <= tolower(*p) && tolower(*p) <= 'f'); *rest = ++p) {
			ret = ret * 16 + to_hexadecimal(*p);
		}
		return ret;
	}

	*rest = p + 1;
	switch (*p) {
		case 'a':
			return '\a';  // Audible bell
		case 'b':
			return '\b';  // Backspace
		case 'f':
			return '\f';  // Form feed - new page
		case 'n':
			return '\n';  // Line feed - new line
		case 'r':
			return '\r';  // Carriage return
		case 't':
			return '\t';  // Horizontal tab
		case 'v':
			return '\v';  // Vertical tab
	}
	return *p;
}

/* -------------------------------------------------------------------- */
/** \name Number Decode Methods
 * This module decodes string specified numbers into an RCCToken,
 * single charachter literals are also considered to be numbers, e.g. 'abc'
 * \{ */

ROSE_INLINE const char *translate_number_from_charachter_literal_type(RCCToken *t, const char *p) {
	if (ELEM(p[0], 'u')) {
		t->type = Tp_Short;	 // UTF-16 character literal
		return p + 1;
	}
	if (ELEM(p[0], 'U')) {
		t->type = Tp_Int;  // UTF-32 character literal
		return p + 1;
	}
	if (ELEM(p[0], 'L')) {
		t->type = Tp_Int;  // Wide character literal
		return p + 1;
	}
	// Someone would assume we use Tp_Char for this but multibyte charachter literals need Tp_Int.
	t->type = Tp_Int;
	return p;
}

ROSE_INLINE bool translate_number_from_charachter_literal(RCCToken *t) {
	const char *p = translate_number_from_charachter_literal_type(t, t->location.p) + 1;

	// Everything is either `long long` or `unsigned long long` (there are no unsigned types for charachter literals)
	long long *l = (long long *)t->payload;
	for (*l = 0; !ELEM(p[0], '\'');) {
		if (ELEM(p[0], '\\')) {
			*l = (*l << 8) + translate_escaped_sequence(&p, p + 1);
		}
		else {
			/**
			 * TODO: Upgrade this so that we can support multiple endianess and string encodings.
			 */
			*l = (*l << 8) + p[0];
			p++;
		}
	}

	return true;
}

ROSE_INLINE bool translate_number_from_floating_point_literal_type(RCCToken *t, const char *p) {
	if (ELEM(p[0], 'f', 'F')) {
		t->type = Tp_Float;
		return p + 1 == t->location.p + t->length;
	}
	if (ELEM(p[0], 'l', 'L')) {
		t->type = Tp_LDouble;
		return p + 1 == t->location.p + t->length;
	}
	t->type = Tp_Double;
	return p == t->location.p + t->length;
}

ROSE_INLINE bool translate_number_from_floating_point_literal(RCCToken *t) {
	const char *p = t->location.p;

	// Everything is stored as `long double` since it can fit everything
	long double *l = (long double *)t->payload;
	*l = strtold(p, (char **)&p);
	/** Negative values should not be casted to a token, but an unary expression instead! */
	ROSE_assert(*l >= 0.0L);

	return translate_number_from_floating_point_literal_type(t, p);
}

ROSE_INLINE const char *translate_number_from_integer_literal_base(RCCToken *t, int *base) {
	if (ELEM(t->location.p[0], '0') && ELEM(t->location.p[1], 'x', 'X')) {
		*base = 16;
		return t->location.p + 2;
	}
	if (ELEM(t->location.p[0], '0')) {
		*base = 8;
		return t->location.p + 1;
	}
	*base = 10;
	return t->location.p;
}

ROSE_INLINE bool translate_number_from_integer_literal_type(RCCToken *t, const char *p) {
	int l = 0, u = 0;
	for (const char *i = p; i != t->location.p + t->length; i++) {
		if (!ELEM(*i, 'l', 'L', 'u', 'U') || u > 1 || l > 2) {
			return false;
		}
		l += ELEM(*i, 'l', 'L');
		u += ELEM(*i, 'u', 'U');
	}
	switch (l) {
		case 0: {
			t->type = (u) ? Tp_UInt : Tp_Int;
		}
			return true;
		case 1: {
			t->type = (u) ? Tp_ULong : Tp_Long;
		}
			return true;
		case 2: {
			t->type = (u) ? Tp_ULLong : Tp_LLong;
		}
			return true;
	}
	return false;
}

ROSE_INLINE bool translate_number_from_integer_literal(RCCToken *t) {
	int base;

	/** Determine the base of the number and skip the first specifiers. */
	const char *s = translate_number_from_integer_literal_base(t, &base), *p = s;
	while (isdigit(*p) || ('a' <= *p && *p <= 'f')) {
		p++;
	}
	/** Determine the type of the number to see signedness below. */
	bool suffix = translate_number_from_integer_literal_type(t, p);

	if (suffix) {
		if (RT_type_is_unsigned(t->type)) {
			unsigned long long *u = (unsigned long long *)t->payload;

			*u = (unsigned long long)strtoull(s, (char **)&p, base);
			return true;
		}
		else {
			signed long long *u = (signed long long *)t->payload;

			*u = (signed long long)strtoll(s, (char **)&p, base);
			/** Negative values should not be casted to a token, but an unary expression instead! */
			ROSE_assert(*u >= 0);
			return true;
		}
	}
	return false;
}

ROSE_INLINE bool translate_number(RCCToken *t) {
	if (token_chrhas(t, '\'')) {
		return translate_number_from_charachter_literal(t);
	}
	if (token_chrhas(t, '.')) {
		return translate_number_from_floating_point_literal(t);
	}
	return translate_number_from_integer_literal(t);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Decode Methods
 * \{ */

ROSE_INLINE const char *translate_string_type(RCCToken *t, const char *p) {
	if (ELEM(p[0], 'L')) {
		t->type = Tp_Short;
		return p + 1;
	}
	t->type = Tp_Char;
	return p;
}

ROSE_INLINE bool translate_string(RCCToken *t) {
	const char *p = translate_string_type(t, t->location.p) + 1;

	size_t index;
	for (index = 0; !ELEM(p[0], '\"'); index++) {
		ROSE_assert(index < TOKEN_MAX_PAYLOAD);	 // We only support string literals up to 256 elements.
		if (ELEM(p[0], '\\')) {
			t->payload[index] = translate_escaped_sequence(&p, p + 1);
		}
		else {
			/**
			 * TODO: Upgrade this so that we can support multiple endianess and string encodings.
			 */
			t->payload[index] = p[0];
			p++;
		}
	}
	t->payload[index] = '\0';

	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

ROSE_INLINE RCCToken *token_new(RCContext *C, int kind, const RCCFile *f, const RCCSLoc *loc, size_t length) {
	/**
	 * We keep the payload zero-initialized,
	 * this way we can do comparisons with the full extend of the payload in `RT_token_match`.
	 */
	RCCToken *t = RT_context_calloc(C, sizeof(RCCToken));

	t->kind = kind;
	t->file = f;
	t->type = NULL;
	t->length = length;

	/** Copy the identifier in the payload, this will be NULL terminated. */
	if (loc) {
		LIB_strncpy(t->payload, ARRAY_SIZE(t->payload), loc->p, length);

		RT_sloc_copy(&t->location, loc);
	}
	return t;
}

RCCToken *RT_token_new_identifier(RCContext *C, const RCCFile *f, const RCCSLoc *loc, size_t length) {
	return token_new(C, TOK_IDENTIFIER, f, loc, length);
}
RCCToken *RT_token_new_keyword(RCContext *C, const RCCFile *f, const RCCSLoc *loc, size_t length) {
	return token_new(C, TOK_KEYWORD, f, loc, length);
}
RCCToken *RT_token_new_punctuator(RCContext *C, const RCCFile *f, const RCCSLoc *loc, size_t length) {
	return token_new(C, TOK_PUNCTUATOR, f, loc, length);
}

RCCToken *RT_token_new_number(RCContext *C, const RCCFile *f, const RCCSLoc *loc, size_t length) {
	RCCToken *t = token_new(C, TOK_NUMBER, f, loc, length);

	if (!translate_number(t)) {
		t = NULL;  // No deallocations, just ignore this pointer!
	}

	return t;
}
RCCToken *RT_token_new_string(RCContext *C, const RCCFile *f, const RCCSLoc *loc, size_t length) {
	RCCToken *t = token_new(C, TOK_STRING, f, loc, length);

	if (!translate_string(t)) {
		t = NULL;  // No deallocations, just ignore this pointer!
	}

	return t;
}
RCCToken *RT_token_new_eof(RCContext *C) {
	RCCToken *t = token_new(C, TOK_EOF, NULL, NULL, 0);

	return t;
}

RCCToken *RT_token_new_name(RCContext *C, const char *name) {
	RCCToken *t = token_new(C, TOK_IDENTIFIER, NULL, NULL, 0);
	
	LIB_strcpy(t->payload, ARRAY_SIZE(t->payload), name);
	
	return t;
}
RCCToken *RT_token_new_size(RCContext *C, unsigned long long size) {
	RCCToken *t = token_new(C, TOK_NUMBER, NULL, NULL, 0);
	
	*(unsigned long long *)t->payload = size;
	
	t->type = Tp_ULLong;
	
	return t;
}
RCCToken *RT_token_new_int(RCContext *C, int value) {
	RCCToken *t = token_new(C, TOK_NUMBER, NULL, NULL, 0);
	
	*(long long *)t->payload = (long long)value;
	
	t->type = Tp_Int;
	
	return t;
}
RCCToken *RT_token_new_llong(RCContext *C, long long value) {
	RCCToken *t = token_new(C, TOK_NUMBER, NULL, NULL, 0);
	
	*(long long *)t->payload = (long long)value;
	
	t->type = Tp_LLong;
	
	return t;
}

RCCToken *RT_token_duplicate(RCContext *C, const RCCToken *token) {
	/**
	 * We keep the payload zero-initialized,
	 * this way we can do comparisons with the full extend of the payload in `RT_token_match`.
	 */
	RCCToken *t = RT_context_calloc(C, sizeof(RCCToken));
	
	memcpy(t, token, sizeof(RCCToken));
	
	t->prev = NULL;
	t->next = NULL;
	
	return t;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Common Methods
 * \{ */

bool RT_token_match(const RCCToken *a, const RCCToken *b) {
	if (a == b) {
		// This also handles the case where both `a` and `b` are NULL.
		return true;
	}
	if (a && b) {
		if (a->kind != b->kind) {
			return false;
		}
		if (a->type != b->type) {
			/**
			 * We use simple types here, so we don't need to go in depth checking the types,
			 * a simple comparisson should suffice.
			 */
			ROSE_assert(!RT_type_same(a->type, b->type));
			return false;
		}
		return memcmp(a->payload, b->payload, TOKEN_MAX_PAYLOAD) == 0;
	}
	return false;
}

bool RT_token_is_identifier(const RCCToken *tok) {
	return tok->kind == TOK_IDENTIFIER;
}
bool RT_token_is_keyword(const RCCToken *tok) {
	return tok->kind == TOK_KEYWORD;
}
bool RT_token_is_punctuator(const RCCToken *tok) {
	return tok->kind == TOK_PUNCTUATOR;
}
bool RT_token_is_number(const RCCToken *tok) {
	return tok->kind == TOK_NUMBER;
}
bool RT_token_is_string(const RCCToken *tok) {
	return tok->kind == TOK_STRING;
}

const char *RT_token_as_string(const RCCToken *tok) {
	ROSE_assert(ELEM(tok->kind, TOK_IDENTIFIER, TOK_KEYWORD, TOK_PUNCTUATOR));

	if (ELEM(tok->kind, TOK_IDENTIFIER, TOK_KEYWORD, TOK_PUNCTUATOR)) {
		/** Null terminated string representing the token. */
		return tok->payload;
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Methods
 * \{ */

const char *RT_token_string(const RCCToken *tok) {
	return tok->payload;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Number Methods
 * \{ */

bool RT_token_is_unsigned(const RCCToken *tok) {
	ROSE_assert(RT_token_is_number(tok) && tok->type && tok->type->is_basic);

	if (tok->type->is_basic) {
		return tok->type->tp_basic.is_unsigned;
	}
	return false;
}
bool RT_token_is_signed(const RCCToken *tok) {
	ROSE_assert(RT_token_is_number(tok) && tok->type && tok->type->is_basic);

	if (tok->type->is_basic) {
		return !tok->type->tp_basic.is_unsigned;
	}
	return false;
}
bool RT_token_is_integer(const RCCToken *tok) {
	ROSE_assert(RT_token_is_number(tok) && tok->type && tok->type->is_basic);

	return ELEM(tok->type->kind, TP_BOOL, TP_CHAR, TP_SHORT, TP_INT, TP_LONG, TP_LLONG);
}
bool RT_token_is_floating(const RCCToken *tok) {
	ROSE_assert(RT_token_is_number(tok) && tok->type && tok->type->is_basic);

	return ELEM(tok->type->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE);
}

long long RT_token_as_integer(const RCCToken *tok) {
	/**
	 * Everything is eventually translated to unsigned long long since there are no negative values.
	 * \note We translate the negative values after the unary expression '-'.
	 */
	ROSE_assert(RT_token_is_integer(tok));
	
	long long payload = *(const long long *)tok->payload;
	
	return payload;
}

int RT_token_as_int(const RCCToken *tok) {
	ROSE_assert(RT_token_is_integer(tok) && RT_token_is_signed(tok) && tok->type->kind == TP_INT);

	int payload = *(const long long *)tok->payload;

	return (int)payload;
}
long RT_token_as_long(const RCCToken *tok) {
	ROSE_assert(RT_token_is_integer(tok) && RT_token_is_signed(tok) && tok->type->kind == TP_LONG);

	long long payload = *(const long long *)tok->payload;

	return (long)payload;
}
long long RT_token_as_llong(const RCCToken *tok) {
	ROSE_assert(RT_token_is_integer(tok) && RT_token_is_signed(tok) && tok->type->kind == TP_LLONG);

	return *(const long long *)tok->payload;
}

unsigned int RT_token_as_uint(const RCCToken *tok) {
	ROSE_assert(RT_token_is_integer(tok) && RT_token_is_unsigned(tok) && tok->type->kind == TP_INT);

	unsigned long long payload = *(const unsigned long long *)tok->payload;

	return (unsigned int)payload;
}
unsigned long RT_token_as_ulong(const RCCToken *tok) {
	ROSE_assert(RT_token_is_integer(tok) && RT_token_is_unsigned(tok) && tok->type->kind == TP_LONG);

	unsigned long long payload = *(const unsigned long long *)tok->payload;

	return (unsigned long)payload;
}
unsigned long long RT_token_as_ullong(const RCCToken *tok) {
	ROSE_assert(RT_token_is_integer(tok) && RT_token_is_unsigned(tok) && tok->type->kind == TP_LLONG);

	return *(const unsigned long long *)tok->payload;
}

long double RT_token_as_scalar(const RCCToken *tok) {
	ROSE_assert(RT_token_is_floating(tok));

	return *(const long double *)tok->payload;
}

float RT_token_as_float(const RCCToken *tok) {
	ROSE_assert(RT_token_is_floating(tok) && tok->type->kind == TP_FLOAT);

	return (float)RT_token_as_scalar(tok);
}
double RT_token_as_double(const RCCToken *tok) {
	ROSE_assert(RT_token_is_floating(tok) && tok->type->kind == TP_DOUBLE);

	return (double)RT_token_as_scalar(tok);
}
long double RT_token_as_ldouble(const RCCToken *tok) {
	ROSE_assert(RT_token_is_floating(tok) && tok->type->kind == TP_LDOUBLE);

	return (long double)RT_token_as_scalar(tok);
}

/** \} */
