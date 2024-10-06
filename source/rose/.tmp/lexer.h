#ifndef __RCC_LEXER_H__
#define __RCC_LEXER_H__

#include "LIB_listbase.h"

#include "source.h"
#include "token.h"
#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCLexer RCCLexer;
 
void RCC_vprintf_error_lexer(RCCLexer *lexer, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, ...);
void RCC_vprintf_warn_lexer(RCCLexer *lexer, const char *loc, ATTR_PRINTF_FORMAT const char *fmt, ...);
 
/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Functions
 * \{ */

/** The lexer will only lex the tokens of a single file! */
RCCLexer *RCC_lexer_new(const RCCFile *file);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Functions
 * \{ */

void RCC_lexer_free(RCCLexer *lexer);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Functions
 * \{ */

const RCCToken *RCC_lexer_token_first(const RCCLexer *lexer);
const RCCToken *RCC_lexer_token_last(const RCCLexer *lexer);

/** \} */

#ifdef __cplusplus
}
#endif

#endif //  __RCC_LEXER_H__
