#ifndef __RCC_PARSER_H__
#define __RCC_PARSER_H__

#include "LIB_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCState RCCState;

typedef struct RCCParser {
	const struct RCCFile *file;

	/**
	 * The tokens extracted in the order they appear in the source.
	 */
	ListBase tokens;
	
	struct RCContext *context;
	struct RCCState *state;
} RCCParser;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCParser *RCC_parser_new(const struct RCCFile *file);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RCC_parser_free(struct RCCParser *parser);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_PARSER_H__
