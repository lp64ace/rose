#ifndef RT_TOKEN_H
#define RT_TOKEN_H

#include "RT_context.h"
#include "RT_source.h"

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

	bool has_leading_space;
	bool beginning_of_line;

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

struct RCCToken *RT_token_new_identifier(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RT_token_new_keyword(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RT_token_new_punctuator(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RT_token_new_number(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RT_token_new_string(struct RCContext *, const struct RCCFile *f, const RCCSLoc *loc, size_t length);
struct RCCToken *RT_token_new_eof(struct RCContext *);

struct RCCToken *RT_token_new_name(struct RCContext *, const char *name);
struct RCCToken *RT_token_new_size(struct RCContext *, unsigned long long size);

struct RCCToken *RT_token_new_int(struct RCContext *, int value);
struct RCCToken *RT_token_new_llong(struct RCContext *, long long value);

struct RCCToken *RT_token_duplicate(struct RCContext *, const struct RCCToken *token);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Common Methods
 * \{ */

bool RT_token_match(const struct RCCToken *, const struct RCCToken *);

bool RT_token_is_identifier(const struct RCCToken *tok);
bool RT_token_is_keyword(const struct RCCToken *tok);
bool RT_token_is_punctuator(const struct RCCToken *tok);
bool RT_token_is_number(const struct RCCToken *tok);
bool RT_token_is_string(const struct RCCToken *tok);

/**
 * Returns the string representing the token, this is only valid for;
 * identifiers, keywrords, punctuators.
 */
const char *RT_token_as_string(const struct RCCToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Methods
 * \{ */

/** Returns the escaped string representing the token. */
const char *RT_token_string(const RCCToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Number Methods
 * \{ */

bool RT_token_is_unsigned(const RCCToken *tok);
bool RT_token_is_signed(const RCCToken *tok);
bool RT_token_is_integer(const RCCToken *tok);
bool RT_token_is_floating(const RCCToken *tok);

long long RT_token_as_integer(const RCCToken *tok);

int RT_token_as_int(const RCCToken *tok);
long RT_token_as_long(const RCCToken *tok);
long long RT_token_as_llong(const RCCToken *tok);

unsigned int RT_token_as_uint(const RCCToken *tok);
unsigned long RT_token_as_ulong(const RCCToken *tok);
unsigned long long RT_token_as_ullong(const RCCToken *tok);

long double RT_token_as_scalar(const RCCToken *tok);

float RT_token_as_float(const RCCToken *tok);
double RT_token_as_double(const RCCToken *tok);
long double RT_token_as_ldouble(const RCCToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

const char *RT_token_working_directory(const RCCToken *tok);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_TOKEN_H
