#ifndef __RCC_TOKEN_H__
#define __RCC_TOKEN_H__

#include "LIB_utildefines.h"

#include "source.h"
#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCCFile;
struct RCCType;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCToken RCCToken;
 
/** \} */

enum {
	TOK_IDENTIFIER,
	TOK_PUNCTUATOR,
	TOK_KEYWORD,
	TOK_STRING,
	TOK_SCALAR,
	TOK_EOF,
};

void RCC_vprintf_error(const char *fname, const char *input, int line, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, va_list varg);
void RCC_vprintf_error_token(RCCToken *token, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, ...);
void RCC_vprintf_warn_token(RCCToken *token, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, ...);

/* -------------------------------------------------------------------- */
/** \name Create Functions
 * \{ */

RCCToken *RCC_token_scalar_new(const RCCFile *file, const char *loc, size_t length);
RCCToken *RCC_token_string_new(const RCCFile *file, const char *loc, size_t length);
RCCToken *RCC_token_punctuator_new(const RCCFile *file, const char *loc, size_t length);

/** Also used for keywords, if the identifier is a keyword the token will automatically update. */
RCCToken *RCC_token_identifier_new(const RCCFile *file, const char *loc, size_t length);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Functions
 * \{ */
 
void RCC_token_free(RCCToken *token);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Update/Query Functions
 * \{ */

int RCC_token_kind(const RCCToken *token);

/** Update the flag regarding the "Has leading space" of the token. */
void RCC_token_set_hls(RCCToken *token, bool test);
/** Update the flag regarding the "Beginning of line" of the token. */
void RCC_token_set_bol(RCCToken *token, bool test);

/** Returns true if the token has the flag "Has leading space" toggled. */
bool RCC_token_hls(const RCCToken *token);
/** Returns true if the token has the flag "Beginning of line" toggled. */
bool RCC_token_bol(const RCCToken *token);

const char *RCC_token_string_value(const RCCToken *token);
long long RCC_token_iscalar_value(const RCCToken *token);
long double RCC_token_fscalar_value(const RCCToken *token);

bool RCC_token_is(const RCCToken *token, const char *what);

RCCToken *RCC_token_next(const RCCToken *token);
RCCToken *RCC_token_prev(const RCCToken *token);
RCCType *RCC_token_type(const RCCToken *token);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utility Functions
 * \{ */

void RCC_token_print(const RCCToken *token);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // __RCC_TOKEN_H__
