#include "COM_context.h"
#include "COM_type.h"
#include "COM_token.h"

#include "LIB_string.h"

#include <ctype.h>
#include <limits.h>

/* -------------------------------------------------------------------- */
/** \name Punctuator
 * \{ */

ROSE_STATIC int punctuator_kind(const char punctuator[]) {
	const char *punctuators[] = {
#define PUNCTUATOR(NAME, LITERAL) [PUNC_##NAME] = LITERAL,
#include "punctuator.inc"
#undef PUNCTUATOR
	};

	for (int index = 0; index < (int)ARRAY_SIZE(punctuators); index++) {
		if (punctuators[index] && STREQ(punctuators[index], punctuator)) {
			return index;
		}
	}
	
	return PUNC_NONE;
}

ROSE_INLINE bool punctuator_translate(CTokenPunctuator *me, const char punctuator[]) {
	LIB_strcpy(me->data, ARRAY_SIZE(me->data), punctuator);
	me->kind = punctuator_kind(punctuator);
	
	return me->kind != PUNC_NONE;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Identifier
 * \{ */

ROSE_STATIC int identifier_kind(const char identifier[]) {
	/**
	 * We could create a hash of a subset of charachters of each identifier to match but this is not clang, 
	 * the usecase of this compiler is very limited to do such missleading optimizations for new people.
	 *
	 * NOTE: If someone actually decides that this is a good idea for our usecase and actually implements it,
	 * beware that there are keywords with less than three charachters!
	 *
	 * EXAMPLE:
	 * #define LITERALASINT(literal) *(int *)literal
	 */
	
	const char *keywords[] = {
#define KEYWORD(NAME, LITERAL) [KEY_##NAME] = LITERAL,
#include "keyword.inc"
#undef KEYWORD
	};
	
	for(int index = 0; index < (int)ARRAY_SIZE(keywords); index++) {
		if (keywords[index] && STREQ(keywords[index], identifier)) {
			return index;
		}
	}
	
	return KEY_NONE;
}

ROSE_INLINE void identifier_translate(CTokenIdentifier *me, const char identifier[]) {
	LIB_strcpy(me->data, ARRAY_SIZE(me->data), identifier);
	
	me->kind = identifier_kind(identifier);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Integer Constant
 * The following code is a direct implementation from the ISO/IEC 9899.
 * \{ */

ROSE_INLINE bool isbase(const char p, int base) {
	if (base <= 10) {
		return '0' <= p && p < '0' + base;
	}
	return ('0' <= p && p <= '9') || ('a' <= tolower(p) && tolower(p) < 'a' + base - 10);
}

ROSE_INLINE int asbase(const char p) {
	if ('0' <= p && p <= '9') {
		return p - '0';
	}
	return tolower(p) - 'a' + 10;
}

ROSE_INLINE int digit_sequence(unsigned long long *out, const char **rest, const char *p, int base) {
	const char *i = p;
	
	for ((*out) = 0; isbase(i[0], base) || ELEM(i[0], '\''); ++i) {
		if (ELEM(i[0], '\'')) {
			if ((i == p) || ELEM(i[1], '\'')) {
				return 0;
			}
			/**
			 * Optional ' between digits for readability, no more than one sequential, 
			 * but the digit sequence cannot begin or have two consequtive '.
			 *
			 * ATTENTION!
			 * Highly unclear code on what it does, the length of the sequence is determined as (i - p).
			 * Incrementing #i alone here would result in longer sequence length than real due to '.
			 */
			p++;
			continue;
		}
		*out = (*out) * base + asbase(i[0]);
	}
	
	return (int)(((*rest) = i) - p);
}

ROSE_INLINE const char *is(const char *p, const char *what) {
	if (STREQLEN(p, what, LIB_strlen(what))) {
		return POINTER_OFFSET(p, LIB_strlen(what));
	}
	return NULL;
}

ROSE_INLINE bool consume(const char **rest, const char *p, const char *what) {
	if ((p = is(p, what))) {
		return (*rest = p) != NULL;
	}
	return false;
}

bool hexadecimal_prefix(const char **rest, const char *p) {
	return consume(rest, p, "0x") || consume(rest, p, "0X");
}

bool binary_prefix(const char **rest, const char *p) {
	return consume(rest, p, "0b") || consume(rest, p, "0B");
}

bool decimal_constant(CTokenConstant *me, const char **rest, const char *p) {
	if (isbase(p[0], 10) && !ELEM(p[0], '0')) {
		/** As long as there are no invalid charachters this will complete nicelly! */
		if (digit_sequence((unsigned long long *)me->payload, rest, p, 10) > 0) {
			unsigned long long c = *(unsigned long long *)me->payload;
			
			do {
				if (IN_RANGE_INCL(c, 0, INT_MAX)) {
					me->typed.type = Tp_Int;
					break;
				}
				if (IN_RANGE_INCL(c, 0, LONG_MAX)) {
					me->typed.type = Tp_Long;
					break;
				}
				if (IN_RANGE_INCL(c, 0, LLONG_MAX)) {
					me->typed.type = Tp_LLong;
					break;
				}
			} while(false);
			
			return true;
		}
	}
	return false;
}

bool octal_constant(CTokenConstant *me, const char **rest, const char *p) {
	if (isbase(p[0], 8) && ELEM(p[0], '0')) {
		/** As long as there are no invalid charachters this will complete nicelly! */
		if (digit_sequence((unsigned long long *)me->payload, rest, p, 8) > 0) {
			unsigned long long c = *(unsigned long long *)me->payload;
			
			do {
				if (IN_RANGE_INCL(c, 0, INT_MAX)) {
					me->typed.type = Tp_Int;
					break;
				}
				if (IN_RANGE_INCL(c, 0, UINT_MAX)) {
					me->typed.type = Tp_UInt;
					break;
				}
				if (IN_RANGE_INCL(c, 0, LONG_MAX)) {
					me->typed.type = Tp_Long;
					break;
				}
				if (IN_RANGE_INCL(c, 0, ULONG_MAX)) {
					me->typed.type = Tp_ULong;
					break;
				}
				if (IN_RANGE_INCL(c, 0, LLONG_MAX)) {
					me->typed.type = Tp_LLong;
					break;
				}
				if (IN_RANGE_INCL(c, 0, ULLONG_MAX)) {
					me->typed.type = Tp_ULLong;
					break;
				}
			} while(false);
			
			return true;
		}
	}
	return false;
}

bool hexadecimal_constant(CTokenConstant *me, const char **rest, const char *p) {
	if (hexadecimal_prefix(&p, p)) {
		/** As long as there are no invalid charachters this will complete nicelly! */
		if (digit_sequence((unsigned long long *)me->payload, rest, p, 16) > 0) {
			unsigned long long c = *(unsigned long long *)me->payload;
			
			do {
				if (IN_RANGE_INCL(c, 0, INT_MAX)) {
					me->typed.type = Tp_Int;
					break;
				}
				if (IN_RANGE_INCL(c, 0, UINT_MAX)) {
					me->typed.type = Tp_UInt;
					break;
				}
				if (IN_RANGE_INCL(c, 0, LONG_MAX)) {
					me->typed.type = Tp_Long;
					break;
				}
				if (IN_RANGE_INCL(c, 0, ULONG_MAX)) {
					me->typed.type = Tp_ULong;
					break;
				}
				if (IN_RANGE_INCL(c, 0, LLONG_MAX)) {
					me->typed.type = Tp_LLong;
					break;
				}
				if (IN_RANGE_INCL(c, 0, ULLONG_MAX)) {
					me->typed.type = Tp_ULLong;
					break;
				}
			} while(false);
			
			return true;
		}
	}
	return false;
}

bool binary_constant(CTokenConstant *me, const char **rest, const char *p) {
	if (binary_prefix(&p, p)) {
		/** As long as there are no invalid charachters this will complete nicelly! */
		if (digit_sequence((unsigned long long *)me->payload, rest, p, 2) > 0) {
			unsigned long long c = *(unsigned long long *)me->payload;
			
			do {
				if (IN_RANGE_INCL(c, 0, INT_MAX)) {
					me->typed.type = Tp_Int;
					break;
				}
				if (IN_RANGE_INCL(c, 0, UINT_MAX)) {
					me->typed.type = Tp_UInt;
					break;
				}
				if (IN_RANGE_INCL(c, 0, LONG_MAX)) {
					me->typed.type = Tp_Long;
					break;
				}
				if (IN_RANGE_INCL(c, 0, ULONG_MAX)) {
					me->typed.type = Tp_ULong;
					break;
				}
				if (IN_RANGE_INCL(c, 0, LLONG_MAX)) {
					me->typed.type = Tp_LLong;
					break;
				}
				if (IN_RANGE_INCL(c, 0, ULLONG_MAX)) {
					me->typed.type = Tp_ULLong;
					break;
				}
			} while(false);
			
			return true;
		}
	}
	return false;
}

bool unsigned_suffix(const char **rest, const char *p) {
	return consume(rest, p, "u") || consume(rest, p, "U");
}

bool long_suffix(const char **rest, const char *p) {
	return consume(rest, p, "l") || consume(rest, p, "L");
}

/** This does not support wb and WB even though it is specified in the C99 ISO! */
bool integer_suffix(CTokenConstant *me, const char **rest, const char *p) {
	if (unsigned_suffix(&p, p)) {
		if (long_suffix(&p, p)) {
			me->typed.type = COM_type_promote(me->typed.type, Tp_Long);
		}
		if (long_suffix(&p, p)) {
			me->typed.type = COM_type_promote(me->typed.type, Tp_LLong);
		}
		me->typed.type = COM_type_unsigned(me->typed.type);
	}
	else {
		if (long_suffix(&p, p)) {
			me->typed.type = COM_type_promote(me->typed.type, Tp_Long);
		}
		if (long_suffix(&p, p)) {
			me->typed.type = COM_type_promote(me->typed.type, Tp_LLong);
		}
		if (unsigned_suffix(&p, p)) {
			me->typed.type = COM_type_unsigned(me->typed.type);
		}
	}
	return ELEM((*rest = p)[0], '\0');
}

bool integer_constant(CTokenConstant *me, const char *p) {
	me->typed.type = Tp_Int;
	
	do {
		if (decimal_constant(me, &p, p)) {
			break;
		}
		if (hexadecimal_constant(me, &p, p)) {
			break;
		}
		if (binary_constant(me, &p, p)) {
			break;
		}
		if (octal_constant(me, &p, p)) {
			break;
		}
		
		/** If none of the above compelete then we failed! */
		return false;
	} while (false);
	
	return integer_suffix(me, &p, p);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Floating Constant
 * The following code is a direct implementation from the ISO/IEC 9899.
 * \{ */

ROSE_INLINE bool negate(const char **rest, const char *p) {
	if (consume(rest, p, "-")) {
		return true;
	}
	if (consume(rest, p, "+")) {
		return false;
	}
	return false;
}

bool fractional_constant(CTokenConstant *me, const char **rest, const char *p) {
	long double *i = (long double *)me->payload;
	
	unsigned long long n;
	unsigned long long m;
	
	int nwidth = digit_sequence(&n, &p, p, 10);
	if (!consume(&p, p, ".")) {
		return false;
	}
	int mwidth = digit_sequence(&m, &p, p, 10);
	if (!((nwidth > 0) || (mwidth > 0))) {
		return false;
	}
	
	(*i) = ((long double)n) + ((long double)m) * powl(10.0L, -mwidth);

	*rest = p;
	return true;
}

bool exponent_part(CTokenConstant *me, const char **rest, const char *p) {
	long double *i = (long double *)me->payload;
	
	if (consume(&p, p, "e") || consume(&p, p, "E")) {
		unsigned long long e;
		if (negate(&p, p)) {
			if (digit_sequence(&e, &p, p, 10) <= 0) {
				return false;
			}
			
			(*i) = (*i) * powl(10.0L, -(long double)e);
		}
		else {
			if (digit_sequence(&e, &p, p, 10) <= 0) {
				return false;
			}
			
			(*i) = (*i) * powl(10.0L, +(long double)e);
		}
	}
	
	*rest = p;
	return true;
}

bool hexadecimal_fractional_constant(CTokenConstant *me, const char **rest, const char *p) {
	long double *i = (long double *)me->payload;
	
	unsigned long long n;
	unsigned long long m;
	
	int nwidth = digit_sequence(&n, &p, p, 16);
	if (!consume(&p, p, ".")) {
		return false;
	}
	int mwidth = digit_sequence(&m, &p, p, 16);
	if (!((nwidth > 0) || (mwidth > 0))) {
		return false;
	}
	
	(*i) = ((long double)n) + ((long double)m) * powl(2.0L, -4.0L * mwidth);

	*rest = p;
	return true;
}

bool hexadecimal_exponent_part(CTokenConstant *me, const char **rest, const char *p) {
	long double *i = (long double *)me->payload;
	
	if (consume(&p, p, "p") || consume(&p, p, "P")) {
		unsigned long long e;
		if (negate(&p, p)) {
			if (digit_sequence(&e, &p, p, 10) <= 0) {
				return false;
			}
			
			(*i) = (*i) * powl(2.0L, -(long double)e);
		}
		else {
			if (digit_sequence(&e, &p, p, 10) <= 0) {
				return false;
			}
			
			(*i) = (*i) * powl(2.0L, +(long double)e);
		}
	}
	
	*rest = p;
	return true;
}

bool decimal_floating_constant(CTokenConstant *me, const char **rest, const char *p) {
	long double *i = (long double *)me->payload;
	
	do {
		if (fractional_constant(me, &p, p)) {
			break;
		}
		unsigned long long n;
		if (digit_sequence(&n, &p, p, 10) > 0) {
			(*i) = ((long double)n);
			break;
		}
		
		return false;
	} while(false);
	
	return exponent_part(me, rest, p);
}

bool hexadecimal_floating_constant(CTokenConstant *me, const char **rest, const char *p) {
	if (hexadecimal_prefix(&p, p)) {
		long double *i = (long double *)me->payload;
	
		do {
			if (hexadecimal_fractional_constant(me, &p, p)) {
				break;
			}
			unsigned long long n;
			if (digit_sequence(&n, &p, p, 16) > 0) {
				(*i) = ((long double)n);
				break;
			}
			
			return false;
		} while(false);
		
		return hexadecimal_exponent_part(me, rest, p);
	}
	return false;
}

/** This does not support df/DF, dd/DD and dl/DL even though it is specified in the C99 ISO! */
bool floating_suffix(CTokenConstant *me, const char **rest, const char *p) {
	do {
		if (consume(&p, p, "f") || consume(&p, p, "F")) {
			me->typed.type = Tp_Float;
			break;
		}
		if (consume(&p, p, "l") || consume(&p, p, "L")) {
			me->typed.type = Tp_LDouble;
			break;
		}
		if (consume(&p, p, "df") || consume(&p, p, "DF")) {
			// me->typed.type = Tp_Decimal32;
			return false;
		}
		if (consume(&p, p, "dd") || consume(&p, p, "DD")) {
			// me->typed.type = Tp_Decimal64;
			return false;
		}
		if (consume(&p, p, "dl") || consume(&p, p, "DL")) {
			// me->typed.type = Tp_Decimal128;
			return false;
		}
	} while(false);
	
	return ELEM((*rest = p)[0], '\0');
}

bool floating_constant(CTokenConstant *me, const char *p) {
	me->typed.type = Tp_Double;
	
	do {
		if (hexadecimal_floating_constant(me, &p, p)) {
			break;
		}
		if (decimal_floating_constant(me, &p, p)) {
			break;
		}
		
		/** If none of the above compelete then we failed! */
		return false;
	} while(false);
	
	return floating_suffix(me, &p, p);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Character Constant
 * The following code is a direct implementation from the ISO/IEC 9899.
 * \{ */

ROSE_INLINE int unescaped(const char **rest, const char *p) {
	if ('0' <= p[0] && p[0] <= '7') {
		char ret = 0;
		for (; ret < 64 && ('0' <= *p && *p <= '7'); *rest = ++p) {
			ret = ret * 8 + (p[0] - '0');
		}
		return (int)ret;
	}
	if (ELEM(p[0], 'x', 'X')) {
		char ret = 0;
		for (*rest = ++p; isbase(p[0], 16); *rest = ++p) {
			ret = ret * 16 + asbase(p[0]);
		}
		return (int)ret;
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

bool encoding_prefix(CTokenTyped *me, const char **rest, const char *p) {
	if (consume(rest, p, "u8")) {
		me->type = Tp_Char;
		return true;
	}
	if (consume(rest, p, "u")) {
		me->type = Tp_Int16;
		return true;
	}
	if (consume(rest, p, "U")) {
		me->type = Tp_Int32;
		return true;
	}
	if (consume(rest, p, "L")) {
		// me->type = Tp_WChar;
		return false;
	}
	return true;
}

int charachter_sequence(CTokenConstant *me, const char **rest, const char *p) {
	unsigned long long *c = (unsigned long long *)me->payload;
	
	const char *i = p;
	for ((*c) = 0; !ELEM(i[0], '\0', '\''); ) {
		if (ELEM(i[0], '\\')) {
			(*c) = ((*c) << 8) + unescaped(&i, i + 1);
		}
		else {
			(*c) = ((*c) << 8) + i[0];
			i++;
		}
	}
	
	return (int)(((*rest) = i) - p);
}

bool character_constant(CTokenConstant *me, const char *p) {
	me->typed.type = Tp_Int;
	
	if (!encoding_prefix(&me->typed, &p, p)) {
		return false;
	}
	if (!consume(&p, p, "\'")) {
		return false;
	}
	if (charachter_sequence(me, &p, p) <= 0) {
		return false;
	}
	if (!consume(&p, p, "\'")) {
		return false;
	}
	
	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Constant
 * \{ */

bool constant_translate(CTokenConstant *me, const char constant[]) {
	if (integer_constant(me, constant)) {
		return true;
	}
	if (floating_constant(me, constant)) {
		return true;
	}
	if (character_constant(me, constant)) {
		return true;
	}
	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Literal
 * \{ */

int s_charachter_sequence(CTokenLiteral *me, const char *p) {
	unsigned char *c = (unsigned char *)me->payload;
	
	int size = 0;
	
	const char *i = p;
	while (!ELEM(i[0], '\0', '\"')) {
		if (ELEM(i[0], '\\')) {
			int a = unescaped(&i, i + 1);
			
			if (c) {
				switch (COM_type_size(me->typed.type)) {
					case 1: *(uint8_t *)c = a; break;
					case 2: *(uint16_t *)c = a; break;
					case 4: *(uint32_t *)c = a; break;
				}
				c = POINTER_OFFSET(c, COM_type_size(me->typed.type));
			}
			
			size += COM_type_size(me->typed.type);
		}
		else {
			if (c) {
				switch (COM_type_size(me->typed.type)) {
					case 1: *(uint8_t *)c = i[0]; break;
					case 2: *(uint16_t *)c = i[0]; break;
					case 4: *(uint32_t *)c = i[0]; break;
				}
				c = POINTER_OFFSET(c, COM_type_size(me->typed.type));
			}
			
			i++;
			
			size += COM_type_size(me->typed.type);
		}
	}
	
	return size;
}

bool string_literal(Compilation *com, CTokenLiteral *me, const char *p) {
	if (!encoding_prefix(&me->typed, &p, p)) {
		return false;
	}
	
	int size = 0;
	
	if (!consume(&p, p, "\"")) {
		return false;
	}
	me->payload = NULL;
	if ((size = s_charachter_sequence(me, p)) < 0) {
		return false;
	}
	me->payload = LIB_memory_arena_calloc(com->blob, size + 4);
	if ((size = s_charachter_sequence(me, p)) < 0) {
		return false;
	}
	if (!consume(&p, p, "\"")) {
		return false;
	}
	
	// me->type = COM_type_bounded_array(me->type, size + 4);
	
	return true;
}

/** \} */

ROSE_INLINE CToken *COM_token_new(Compilation *com, int kind) {
	CToken *token = (CToken *)LIB_memory_pool_malloc(com->token);
	if (token) {
		token->kind = kind;
	}
	return token;
}

CToken *COM_token_punct(Compilation *com, const char punctuator[]) {
	CToken *token = COM_token_new(com, TOK_PUNCTUATOR);
	if (token) {
		if (!punctuator_translate(&token->punctuator, punctuator)) {
			/**
			 * The deallocation will be at the very end, so the memory will not even get fragmented!
			 */
			LIB_memory_pool_free(com->token, token);
			return NULL;
		}
	}
	return token;
}

CToken *COM_token_named(Compilation *com, const char identifier[]) {
	CToken *token = COM_token_new(com, TOK_IDENTIFIER);
	if (token) {
		identifier_translate(&token->identifier, identifier);
	}
	return token;
}

CToken *COM_token_const(Compilation *com, const char constant[]) {
	CToken *token = COM_token_new(com, TOK_CONSTANT);
	if (token) {
		if (!constant_translate(&token->constant, constant)) {
			/**
			 * The deallocation will be at the very end, so the memory will not even get fragmented!
			 */
			LIB_memory_pool_free(com->token, token);
			return NULL;
		}
	}
	return token;
}

CToken *COM_token_blob(Compilation *com, const char blob[]) {
	CToken *token = COM_token_new(com, TOK_LITERAL);
	if (token) {
		if (!string_literal(com, &token->literal, blob)) {
			/**
			 * The deallocation will be at the very end, so the memory will not even get fragmented!
			 */
			LIB_memory_pool_free(com->token, token);
			return NULL;
		}
	}
	return token;
}

CToken *COM_token_eof(Compilation *com) {
	return COM_token_new(com, TOK_EOF);
}
