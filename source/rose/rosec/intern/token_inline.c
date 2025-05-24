#ifndef TOKEN_INLINE_C
#define TOKEN_INLINE_C

#include "COM_token.h"

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE bool COM_token_is_punct(const CToken *token) {
	return token->kind == TOK_PUNCTUATOR;
}
ROSE_INLINE bool COM_token_is_named(const CToken *token) {
	return token->kind == TOK_IDENTIFIER;
}
ROSE_INLINE bool COM_token_is_const(const CToken *token) {
	return token->kind == TOK_CONSTANT;
}
ROSE_INLINE bool COM_token_is_blob(const CToken *token) {
	return token->kind == TOK_LITERAL;
}
ROSE_INLINE bool COM_token_is_eof(const CToken *token) {
	return token->kind == TOK_EOF;
}

ROSE_INLINE const char *COM_token_as_string(const CToken *token) {
	if (!COM_token_is_string(token)) {
		return "(nill)";
	}
	switch(token->kind) {
		case TOK_PUNCTUATOR: return token->punctuator.data;
		case TOK_IDENTIFIER: return token->identifier.data;
		/** This is higly unsafe but for now this will do, we do not compile user code! */
		case TOK_LITERAL: return token->literal.payload;
	}
	return "(null)";
}
ROSE_INLINE bool COM_token_is_string(const CToken *token) {
	return ELEM(token->kind, TOK_PUNCTUATOR, TOK_IDENTIFIER, TOK_LITERAL);
}

ROSE_INLINE bool COM_token_is_keyword(const CToken *token) {
	if (COM_token_is_named(token)) {
		return !ELEM(token->identifier.kind, KEY_NONE);
	}
	return false;
}

#ifdef __cplusplus
}
#endif

#endif