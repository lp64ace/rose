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

typedef struct RTToken {
	struct RTToken *prev, *next;

	int kind;

	RTSLoc location;
	const struct RTFile *file;
	const struct RTType *type;

	bool has_leading_space;
	bool beginning_of_line;

	size_t length;

	char payload[TOKEN_MAX_PAYLOAD];
} RTToken;

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

struct RTToken *RT_token_new_identifier(struct RTContext *, const struct RTFile *f, const RTSLoc *loc, size_t length);
struct RTToken *RT_token_new_keyword(struct RTContext *, const struct RTFile *f, const RTSLoc *loc, size_t length);
struct RTToken *RT_token_new_punctuator(struct RTContext *, const struct RTFile *f, const RTSLoc *loc, size_t length);
struct RTToken *RT_token_new_number(struct RTContext *, const struct RTFile *f, const RTSLoc *loc, size_t length);
struct RTToken *RT_token_new_string(struct RTContext *, const struct RTFile *f, const RTSLoc *loc, size_t length);
struct RTToken *RT_token_new_eof(struct RTContext *);

struct RTToken *RT_token_new_virtual_keyword(struct RTContext *, const char *keyword);
struct RTToken *RT_token_new_virtual_identifier(struct RTContext *, const char *name);
struct RTToken *RT_token_new_virtual_punctuator(struct RTContext *, const char *punc);
struct RTToken *RT_token_new_virtual_size(struct RTContext *, unsigned long long size);
struct RTToken *RT_token_new_virtual_int(struct RTContext *, int value);
struct RTToken *RT_token_new_virtual_llong(struct RTContext *, long long value);

struct RTToken *RT_token_duplicate(struct RTContext *, const struct RTToken *token);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Common Methods
 * \{ */

bool RT_token_match(const struct RTToken *, const struct RTToken *);

bool RT_token_is_identifier(const struct RTToken *tok);
bool RT_token_is_keyword(const struct RTToken *tok);
bool RT_token_is_punctuator(const struct RTToken *tok);
bool RT_token_is_number(const struct RTToken *tok);
bool RT_token_is_string(const struct RTToken *tok);

/**
 * Returns the string representing the token, this is only valid for;
 * identifiers, keywrords, punctuators.
 */
const char *RT_token_as_string(const struct RTToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name String Methods
 * \{ */

/** Returns the escaped string representing the token. */
const char *RT_token_string(const RTToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Number Methods
 * \{ */

bool RT_token_is_unsigned(const RTToken *tok);
bool RT_token_is_signed(const RTToken *tok);
bool RT_token_is_integer(const RTToken *tok);
bool RT_token_is_floating(const RTToken *tok);

long long RT_token_as_integer(const RTToken *tok);

int RT_token_as_int(const RTToken *tok);
long RT_token_as_long(const RTToken *tok);
long long RT_token_as_llong(const RTToken *tok);

unsigned int RT_token_as_uint(const RTToken *tok);
unsigned long RT_token_as_ulong(const RTToken *tok);
unsigned long long RT_token_as_ullong(const RTToken *tok);

long double RT_token_as_scalar(const RTToken *tok);

float RT_token_as_float(const RTToken *tok);
double RT_token_as_double(const RTToken *tok);
long double RT_token_as_ldouble(const RTToken *tok);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

const char *RT_token_working_directory(const RTToken *tok);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_TOKEN_H
