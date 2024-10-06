#include "MEM_guardedalloc.h"

#include "LIB_string.h"

#include "source.h"
#include "token.h"
#include "type.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCToken {
	struct RCCToken *prev, *next;
	
	/** How this RCCToken should be treated, see the TOK_XXX, enum! */
	int kind;
	
	long long integer;
	long double scalar;
	char *literal;
	
	const RCCFile *file;
	/** The location this token was found in the file. */
	const char *loc;
	/** The length of the token in charachters. */
	size_t length;
	
	int flag;
	
	/** Mainly used for expanded macros, this is the original token. */
	const RCCToken *origin;
	/** Mainly used for literals, this is the type of the token. */
	RCCType *type;
} RCCToken;

enum {
	TOKEN_BOL = (1 << 0),
	TOKEN_HLS = (1 << 1),
};

/** \} */

void RCC_vprintf_error(const char *fname, const char *input, int line, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, va_list varg) {
	const char *begin = loc;
	while(input < begin && begin[-1] != '\n') {
		begin--;
	}
	
	const char *end = loc;
	while(*end != '\0' && *end != '\n') {
		end++;
	}
	
	int indent = fprintf(stderr, "%s:%d: ", fname, line);
	fprintf(stderr, "%.*s\n", (int)(end - begin), begin);
	
	fprintf(stderr, "%*s^", (int)(loc - begin) + indent, "");
	vfprintf(stderr, fmt, varg);
	fprintf(stderr, "\n");
}
void RCC_vprintf_error_token(RCCToken *token, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, ...) {
	int line = 1;
	for(const char *p = RCC_file_content(token->file); p < loc; p++) {
		line += (*p == '\n');
	}
	
	va_list varg;
	va_start(varg, fmt);
	RCC_vprintf_error(RCC_file_name(token->file), RCC_file_content(token->file), line, loc, fmt, varg);
	va_end(varg);
}
void RCC_vprintf_warn_token(RCCToken *token, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, ...) {
	loc = (loc) ? loc : token->loc;

	int line = 1;
	for(const char *p = RCC_file_content(token->file); p < loc; p++) {
		line += (*p == '\n');
	}
	
	va_list varg;
	va_start(varg, fmt);
	RCC_vprintf_error(RCC_file_name(token->file), RCC_file_content(token->file), line, loc, fmt, varg);
	va_end(varg);
}

/* -------------------------------------------------------------------- */
/** \name Scalar Literal
 * \{ */

ROSE_INLINE bool RCC_token_handle_real(RCCToken *token) {
	const char *p = token->loc;
	
	token->scalar = strtold(token->loc, &p);
	
	size_t l = 0, f = 0;
	for(token->type = NULL; p < token->loc + token->length && ELEM(*p, 'l', 'L', 'f', 'F'); p++) {
		l += ELEM(*p, 'l', 'L');
		f += ELEM(*p, 'f', 'F');
	}
	
	if(p != token->loc + token->length || l > 1 || f > 1 || (l && f)) {
		RCC_vprintf_error_token(token, p, "invalid suffix on integer constant");
		return false;
	}
	
	token->integer = (long long)token->scalar;
	
	if(f) {
		token->type = RCC_type_duplicate(Tp_Float);
	}
	else {
		token->type = (l) ? RCC_type_duplicate(Tp_LDouble) : RCC_type_duplicate(Tp_Double);
	}
	
	return true;
}

ROSE_INLINE bool isbasedigit(int base, int d) {
	return IN_RANGE_INCL(d, '0', ROSE_MIN('0' + base, '9')) || ((tolower(d) - 'a' + 10) < base);
}

ROSE_INLINE int asbasedigit(int base, int d) {
	if(IN_RANGE_INCL(d, '0', ROSE_MIN('0' + base, '9'))) {
		return d - '0';
	}
	return ((tolower(d) - 'a' + 10) < base) ? (tolower(d) - 'a' + 10) : 0;
}

ROSE_INLINE const char *RCC_token_handle_integer_base(RCCToken *token, int base, const char *p) {
	const char *q = p;
	for(token->integer = 0; q < token->loc + token->length && isbasedigit(base, *q); q++) {
		token->integer = token->integer * base + asbasedigit(base, *q);
	}
	token->scalar = (long double)token->integer;
	return q;
}

ROSE_INLINE bool RCC_token_handle_integer(RCCToken *token) {
	const char *p = token->loc;
	if(ELEM(p[0], '0')) {
		bool hex = ELEM(p[1], 'x', 'X');
		/**
		 * This expression will produce the following result based on hex;
		 * - `RCC_token_handle_integer_base(token, 16, p + 2)`, hex = true!
		 * - `RCC_token_handle_integer_base(token, 8, p + 1)`, hex = false!
		 */
		p = RCC_token_handle_integer_base(token, 8 * (hex + 1), p + (hex + 1));
	}
	else {
		p = RCC_token_handle_integer_base(token, 10, p + 0);
	}
	
	size_t l = 0, u = 0;
	for(token->type = NULL; p < token->loc + token->length && ELEM(*p, 'l', 'L', 'u', 'U'); p++) {
		l += ELEM(*p, 'l', 'L');
		u += ELEM(*p, 'u', 'U');
	}
	
	if(p != token->loc + token->length || l > 2 || u > 1) {
		RCC_vprintf_error_token(token, p, "invalid suffix on integer constant");
		return false;
	}
	
	switch(l) {
		case 0:
		case 1: {
			token->type = (u > 0) ? RCC_type_duplicate(Tp_UInt) : RCC_type_duplicate(Tp_Int);
		} break;
		case 2: {
			token->type = (u > 0) ? RCC_type_duplicate(Tp_ULong) : RCC_type_duplicate(Tp_Long);
		} break;
	}
	
	return true;
}

ROSE_INLINE bool RCC_token_handle_scalar(RCCToken *token) {
	if(LIB_strnext(token->loc, token->loc + token->length, token->loc, '.') != token->loc + token->length) {
		/** This has a decimal point! */
		return RCC_token_handle_real(token);
	}
	if(LIB_strnext(token->loc, token->loc + token->length, token->loc, 'e') != token->loc + token->length) {
		/** This has an exponent part! */
		return RCC_token_handle_real(token);
	}
	/** Default fallback! */
	return RCC_token_handle_integer(token);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Literal
 * \{ */

ROSE_INLINE int escaped(const char **end, const char *p) {
	int ret = 0;
	if(ELEM(*p, '0', 'x', 'X')) {
		int base = (*p == 'x') ? 16 : 8;
		for(*end = p + 1; isbasedigit(base, **end); (*end)++) {
			ret = ret * base + asbasedigit(base, **end);
		}
		return ret;
	}
	
	*end = p + 1;
	
	switch(*p) {
		case 'a': return '\a';
		case 'b': return '\b';
		case 't': return '\t';
		case 'n': return '\n';
		case 'v': return '\v';
		case 'f': return '\f';
		case 'r': return '\r';
	}
	return *p;
}

ROSE_INLINE bool RCC_token_handle_char_literal(RCCToken *token, const char *p) {
	const char *end = token->loc + token->length;
	token->literal = MEM_callocN(LIB_strprev(p, end, end - 1, '\'') - p, "RCCCharLiteral");
	
	const char *q = p;
	while(!ELEM(*q, '\'')) {
		int i = ELEM(*q, '\\') ? escaped(&q, q) : *q++;
		/** Let's not fuck with endianess here... things will get very out of hand, very quickly! */
		token->integer = (token->integer << 8) + i;
		token->literal[q - p - 1] = i;
		
		if(i >= 256) {
			RCC_vprintf_error_token(token, q, "invalid charachter in char literal constant");
			return false;
		}
	}
	
	token->scalar = (long double)token->integer;
	
	if(q != end) {
		RCC_vprintf_error_token(token, q, "invalid suffix on string literal constant");
		return false;
	}
	
	return true;
}

ROSE_INLINE bool RCC_token_handle_string_literal(RCCToken *token, const char *p) {
	const char *end = token->loc + token->length;
	token->literal = MEM_callocN(LIB_strprev(p, end, end - 1, '\"') - p, "RCCStringLiteral");
	
	const char *q = p;
	while(!ELEM(*q, '\"')) {
		int i = ELEM(*q, '\\') ? escaped(&q, q) : *q++;
		token->literal[q - p - 1] = i;
		
		if(i >= 256) {
			RCC_vprintf_error_token(token, q, "invalid charachter in string literal constant");
			return false;
		}
	}
	
	if(q != end) {
		RCC_vprintf_error_token(token, q, "invalid suffix on string literal constant");
		return false;
	}
	
	return true;
}

ROSE_INLINE bool RCC_token_handle_string(RCCToken *token) {
	const char *p = token->loc;
	
	if(ELEM(*p, 'l', 'L')) {
		token->type = RCC_type_duplicate(Tp_Int), p++;
	}
	else {
		token->type = RCC_type_duplicate(Tp_Char);
	}
	
	if(ELEM(*p, '\'')) {
		return RCC_token_handle_char_literal(token, p + 1);
	}
	if(ELEM(*p, '\"')) {
		return RCC_token_handle_string_literal(token, p + 1);
	}
	
	RCC_vprintf_error_token(token, p, "invalid prefix on string literal constant");
	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Punctuator
 * \{ */

ROSE_INLINE bool RCC_token_handle_punctuator(RCCToken *token) {
	if((token->literal = LIB_strndupN(token->loc, token->length)) == NULL) {
		RCC_vprintf_error_token(token, token->loc, "internal compiler error, failed to allocate token");
		return false;
	}
	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Identifier
 * \{ */

ROSE_INLINE bool isid(char what, bool leading) {
	return isalpha(what) || ELEM(what, '_') || (!leading && '0' <= what && what <= '9');
}

ROSE_INLINE bool RCC_token_handle_identifier(RCCToken *token) {
	static char *kw[] = {
		"return", "if", "else", "for", "while", "int", "sizeof", "char", "struct", "union", 
		"short", "long", "void", "typedef", "_Bool", "enum", "static", "goto", "break", 
		"continue", "switch", "case", "default", "extern", "_Alignof", "_Alignas", "do", 
		"signed", "unsigned", "const", "volatile", "auto", "register", "restrict", 
		"__restrict", "__restrict__", "_Noreturn", "float", "double", "typeof", "asm", 
		"_Thread_local", "__thread", "_Atomic", "__attribute__",
	};
	
	for(size_t index = 0; index < ARRAY_SIZE(kw); index++) {
		size_t length = LIB_strlen(kw[index]);
		if(STREQLEN(token->loc, kw[index], length) && !isid(token->loc[length], length == 0)) {
			token->kind = TOK_KEYWORD;
		}
	}
	
	if((token->literal = LIB_strndupN(token->loc, token->length)) == NULL) {
		RCC_vprintf_error_token(token, token->loc, "internal compiler error, failed to allocate token");
		return false;
	}
	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Functions
 * \{ */

ROSE_INLINE RCCToken *RCC_token_new(int kind, const RCCFile *file, const char *loc, size_t length) {
	RCCToken *token = MEM_callocN(sizeof(RCCToken), "RCCToken");
	
	token->kind = kind;
	token->file = file;
	token->loc = loc;
	token->length = length;
	
	return token;
}

RCCToken *RCC_token_scalar_new(const RCCFile *file, const char *loc, size_t length) {
	RCCToken *token = RCC_token_new(TOK_SCALAR, file, loc, length);
	if(!RCC_token_handle_scalar(token)) {
		RCC_token_free(token);
		return NULL;
	}
	return token;
}

RCCToken *RCC_token_string_new(const RCCFile *file, const char *loc, size_t length) {
	RCCToken *token = RCC_token_new(TOK_STRING, file, loc, length);
	if(!RCC_token_handle_string(token)) {
		RCC_token_free(token);
		return NULL;
	}
	return token;
}

RCCToken *RCC_token_punctuator_new(const RCCFile *file, const char *loc, size_t length) {
	RCCToken *token = RCC_token_new(TOK_PUNCTUATOR, file, loc, length);
	if(!RCC_token_handle_punctuator(token)) {
		RCC_token_free(token);
		return NULL;
	}
	return token;
}

RCCToken *RCC_token_identifier_new(const RCCFile *file, const char *loc, size_t length) {
	RCCToken *token = RCC_token_new(TOK_IDENTIFIER, file, loc, length);
	if(!RCC_token_handle_identifier(token)) {
		RCC_token_free(token);
		return NULL;
	}
	return token;
}

/** \} */


/* -------------------------------------------------------------------- */
/** \name Delete Functions
 * \{ */
 
void RCC_token_free(RCCToken *token) {
	MEM_SAFE_FREE(token->literal);
	if(token->type) {
		RCC_type_free(token->type);
	}
	MEM_freeN(token);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Update/Query Functions
 * \{ */

int RCC_token_kind(const RCCToken *token) {
	return token->kind;
}

void RCC_token_set_hls(RCCToken *token, bool test) {
	if(test) {
		token->flag |= TOKEN_HLS;
	} else {
		token->flag &= ~TOKEN_HLS;
	}
}

void RCC_token_set_bol(RCCToken *token, bool test) {
	if(test) {
		token->flag |= TOKEN_BOL;
	} else {
		token->flag &= ~TOKEN_BOL;
	}
}

bool RCC_token_hls(const RCCToken *token) {
	return (token->flag & TOKEN_HLS) != 0;
}

bool RCC_token_bol(const RCCToken *token) {
	return (token->flag & TOKEN_BOL) != 0;
}

const char *RCC_token_string_value(const RCCToken *token) {
	return token->literal;
}
long long RCC_token_iscalar_value(const RCCToken *token) {
	return token->integer;
}
long double RCC_token_fscalar_value(const RCCToken *token) {
	return token->scalar;
}

bool RCC_token_is(const RCCToken *token, const char *what) {
	if(token->kind == TOK_KEYWORD || token->kind == TOK_IDENTIFIER || token->kind == TOK_PUNCTUATOR) {
		return STREQ(token->literal, what);
	}
	return false;
}

RCCToken *RCC_token_next(const RCCToken *token) {
	return token->next;
}
RCCToken *RCC_token_prev(const RCCToken *token) {
	return token->prev;
}
RCCType *RCC_token_type(const RCCToken *token) {
	return token->type;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utility Functions
 * \{ */

ROSE_INLINE const char *RCC_token_kind_name(int kind) {
	const char *names[] = {
		[TOK_IDENTIFIER] = "identifier",
		[TOK_PUNCTUATOR] = "punctruator",
		[TOK_KEYWORD] = "kerword",
		[TOK_STRING] = "string",
		[TOK_SCALAR] = "scalar",
		[TOK_EOF] = "eof",
	};

	return names[kind];
}

void RCC_token_print(const RCCToken *token) {
	fprintf(stdout, "{ kind: %12s, length: %zu }", RCC_token_kind_name(token->kind), token->length);
	if (token->kind == TOK_SCALAR) {
		fprintf(stdout, " value: %llu(%Lf)", token->integer, token->scalar);
	}
	if (token->literal) {
		fprintf(stdout, " value: %s", token->literal);
	}
}

/** \} */
