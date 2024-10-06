#ifndef __RCC_TOKEN_H__
#define __RCC_TOKEN_H__

#include "context.h"
#include "source.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

#define TOKEN_MAX_PAYLOAD 256

typedef struct RCCToken {
	struct RCCToken *prev, *next;

	int kind;

	RCCSLoc location;
	const struct RCCFile *file;
	const struct RCCType *type;

	size_t length;

	char payload[TOKEN_MAX_PAYLOAD];
} RCCToken;

enum {
	TOK_IDENTIFIER,
	TOK_KEYWORD,
	TOK_PUNCTUATOR,
	TOK_NUMBER,
	TOK_STRING,
	TOK_EOF,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCToken *RCC_token_new_identifier(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RCC_token_new_keyword(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RCC_token_new_punctuator(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RCC_token_new_number(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RCC_token_new_string(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RCC_token_new_eof(struct RCContext *);

struct RCCToken *RCC_token_auto(struct RCContext *, const struct RCCFile *f, const char **rest, const RCCSLoc *loc);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Common Methods
 * \{ */

bool RCC_token_match(const struct RCCToken *, const struct RCCToken *);

bool RCC_token_is_identifier(const struct RCCToken *tok);
bool RCC_token_is_keyword(const struct RCCToken *tok);
bool RCC_token_is_punctuator(const struct RCCToken *tok);
bool RCC_token_is_number(const struct RCCToken *tok);
bool RCC_token_is_string(const struct RCCToken *tok);

/**
 * Returns the string representing the token, this is only valid for;
 * identifiers, keywrords, punctuators.
 */
const char *RCC_token_as_string(const struct RCCToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Methods
 * \{ */

/** Returns the escaped string representing the token. */
const char *RCC_token_string(const RCCToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Number Methods
 * \{ */

bool RCC_token_is_unsigned(const RCCToken *tok);
bool RCC_token_is_signed(const RCCToken *tok);
bool RCC_token_is_integer(const RCCToken *tok);
bool RCC_token_is_floating(const RCCToken *tok);

int RCC_token_as_int(const RCCToken *tok);
long RCC_token_as_long(const RCCToken *tok);
long long RCC_token_as_llong(const RCCToken *tok);

unsigned int RCC_token_as_uint(const RCCToken *tok);
unsigned long RCC_token_as_ulong(const RCCToken *tok);
unsigned long long RCC_token_as_ullong(const RCCToken *tok);

long double RCC_token_as_scalar(const RCCToken *tok);

float RCC_token_as_float(const RCCToken *tok);
double RCC_token_as_double(const RCCToken *tok);
long double RCC_token_as_ldouble(const RCCToken *tok);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_TOKEN_H__
