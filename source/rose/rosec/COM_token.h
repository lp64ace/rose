#ifndef COM_TOKEN_H
#define COM_TOKEN_H

#include "COM_type.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CType;
struct Compilation;

typedef struct CTokenPunctuator {
	char data[64];

	int kind;
} CTokenPunctuator;

enum {
	PUNC_NONE,

#define PUNCTUATOR(NAME, LITERAL) PUNC_##NAME,
#include "intern/punctuator.inc"
#undef PUNCTUATOR
};

typedef struct CTokenIdentifier {
	char data[64];

	int kind;
} CTokenIdentifier;

enum {
	KEY_NONE,

#define KEYWORD(NAME, LITERAL) KEY_##NAME,
#include "intern/keyword.inc"
#undef KEYWORD
};

typedef struct CTokenTyped {
	const CType *type;
} CTokenTyped;

typedef struct CTokenConstant {
	CTokenTyped typed;
	
	/**
	 * Raw byte representation of the constant's value.
	 *
	 * Note: This is NOT the source code representation of the value.
	 * Instead, the stored value depends on 'kind':
	 *
	 * CCHARACTER, a single unsigned long long denoting the value, accessible as *(const unsigned long long *)payload.
	 * CINTEGER, a single unsigned long long denoting the value, accessible as *(const unsigned long long *)payload.
	 * CFLOATING, a single long double denoting the value, accessible as *(const long double *)payload.
	 */
	unsigned char payload[16];

	int kind;
} CTokenConstant;

ROSE_STATIC_ASSERT(sizeof(long long) <= sizeof(((CTokenConstant *)0)->payload), "CCHARACTER will not fit in CTokenConstant");
ROSE_STATIC_ASSERT(sizeof(long long) <= sizeof(((CTokenConstant *)0)->payload), "CINTEGER will not fit in CTokenConstant");
ROSE_STATIC_ASSERT(sizeof(long double) <= sizeof(((CTokenConstant *)0)->payload), "CFLOATING will not fit in CTokenConstant");

enum {
	CCHARACTER,
	CINTEGER,
	CFLOATING,
};

typedef struct CTokenLiteral {
	CTokenTyped typed;
	
	/**
	 * Since string literals are the only tokens, constants that do not have a max size,
	 * we store them in a dynamically allocated array, they are allocated through a memory arena in the compilation context.
	 *
	 * \see Compilation::blob
	 */
	char *payload;
} CTokenLiteral;

typedef struct CToken {
	struct CToken *prev, *next;
	
	int kind;
	
	union {
		struct CTokenPunctuator punctuator;
		struct CTokenIdentifier identifier;
		struct CTokenConstant constant;
		struct CTokenLiteral literal;
	};
} CToken;

enum {
	TOK_PUNCTUATOR,
	TOK_IDENTIFIER,
	TOK_CONSTANT,
	TOK_LITERAL,
	TOK_EOF,
};

CToken *COM_token_punct(Compilation *, const char punctuator[]);
CToken *COM_token_named(Compilation *, const char identifier[]);
CToken *COM_token_const(Compilation *, const char constant[]);
CToken *COM_token_blob(Compilation *, const char blob[]);
CToken *COM_token_eof(Compilation *);

ROSE_INLINE bool COM_token_is_punct(const CToken *token);
ROSE_INLINE bool COM_token_is_named(const CToken *token);
ROSE_INLINE bool COM_token_is_const(const CToken *token);
ROSE_INLINE bool COM_token_is_blob(const CToken *token);
ROSE_INLINE bool COM_token_is_eof(const CToken *token);

ROSE_INLINE bool COM_token_is_keyword(const CToken *token);

ROSE_INLINE const char *COM_token_as_string(const CToken *token);
ROSE_INLINE bool COM_token_is_string(const CToken *token);

#ifdef __cplusplus
}
#endif

#include "intern/token_inline.c"

#endif
