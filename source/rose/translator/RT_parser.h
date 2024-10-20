#ifndef RT_PARSER_H
#define RT_PARSER_H

#include "LIB_listbase.h"

/** We expose internal includes, so keep these paths relative! */
#include "./intern/ast/type.h"
#include "./intern/ast/node.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCState RCCState;

typedef struct DeclInfo {
	bool is_typedef;
	bool is_static;
	bool is_extern;
	bool is_inline;
	
	int align;
} DeclInfo;

typedef struct RCConfiguration {
	const struct RCCType *tp_size;
	const struct RCCType *tp_enum;

	int align;
} RCConfiguration;

typedef struct RCCParser {
	const struct RCCFile *file;

	/**
	 * The tokens extracted in the order they appear in the source.
	 */
	ListBase tokens;
	
	struct RCContext *context;
	struct RCCState *state;
	
	RCConfiguration configuration;
	
	ListBase nodes;
} RCCParser;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCParser *RT_parser_new(const struct RCCFile *file);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RT_parser_free(struct RCCParser *parser);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

bool RT_parser_do(struct RCCParser *parser);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

const struct RCCNode *RT_parser_conditional(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);

const struct RCCType *RT_parser_struct(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCType *RT_parser_enum(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);

const struct RCCType *RT_parser_declspec(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token, DeclInfo *info);
const struct RCCType *RT_parser_typename(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCType *RT_parser_pointers(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token, const RCCType *source);
const struct RCCType *RT_parser_abstract(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token, const RCCType *source);
const struct RCCType *RT_parser_funcparams(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token, const RCCType *result);
const struct RCCType *RT_parser_dimensions(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token, const RCCType *element);

struct RCCObject *RT_parser_declarator(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token, const struct RCCType *source);

const struct RCCNode *RT_parser_assign(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_cast(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_unary(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_postfix(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_mul(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_add(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_shift(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_relational(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_equality(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_bitand(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_bitxor(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_bitor(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_logand(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_logor(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_expr(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);
const struct RCCNode *RT_parser_stmt(struct RCCParser *P, struct RCCToken **rest, struct RCCToken *token);

unsigned long long RT_parser_size(struct RCCParser *P, const struct RCCType *type);
unsigned long long RT_parser_offsetof(struct RCCParser *P, const struct RCCType *type, const struct RCCField *field);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_PARSER_H
