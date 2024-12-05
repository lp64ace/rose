#ifndef RT_PARSER_H
#define RT_PARSER_H

#include "LIB_listbase.h"

/** We expose internal includes, so keep these paths relative! */
#include "./intern/ast/node.h"
#include "./intern/ast/type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RTState RTState;

typedef struct DeclInfo {
	bool is_typedef;
	bool is_static;
	bool is_extern;
	bool is_inline;

	int align;
} DeclInfo;

typedef struct RTConfiguration {
	const struct RTType *tp_size;
	const struct RTType *tp_enum;

	int align;
} RTConfiguration;

typedef struct RTParser {
	const struct RTFile *file;

	/**
	 * The tokens extracted in the order they appear in the source.
	 */
	ListBase tokens;

	struct RTContext *context;
	struct RTState *state;

	RTConfiguration configuration;

	ListBase nodes;
} RTCParser;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RTParser *RT_parser_new(const struct RTFile *file);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RT_parser_free(struct RTParser *parser);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

bool RT_parser_do(struct RTParser *parser);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

const struct RTNode *RT_parser_conditional(struct RTParser *P, struct RTToken **rest, struct RTToken *token);

const struct RTType *RT_parser_struct(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTType *RT_parser_enum(struct RTParser *P, struct RTToken **rest, struct RTToken *token);

const struct RTType *RT_parser_declspec(struct RTParser *P, struct RTToken **rest, struct RTToken *token, DeclInfo *info);
const struct RTType *RT_parser_typename(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTType *RT_parser_pointers(struct RTParser *P, struct RTToken **rest, struct RTToken *token, const RTType *source);
const struct RTType *RT_parser_abstract(struct RTParser *P, struct RTToken **rest, struct RTToken *token, const RTType *source);
const struct RTType *RT_parser_funcparams(struct RTParser *P, struct RTToken **rest, struct RTToken *token, const RTType *result);
const struct RTType *RT_parser_dimensions(struct RTParser *P, struct RTToken **rest, struct RTToken *token, const RTType *element);

struct RTObject *RT_parser_declarator(struct RTParser *P, struct RTToken **rest, struct RTToken *token, const struct RTType *source);

const struct RTNode *RT_parser_assign(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_cast(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_unary(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_postfix(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_mul(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_add(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_shift(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_relational(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_equality(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_bitand(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_bitxor(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_bitor(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_logand(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_logor(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_expr(struct RTParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_stmt(struct RTParser *P, struct RTToken **rest, struct RTToken *token);

unsigned long long RT_parser_alignof(struct RTParser *P, const struct RTType *type);
unsigned long long RT_parser_size(struct RTParser *P, const struct RTType *type);
unsigned long long RT_parser_offsetof(struct RTParser *P, const struct RTType *type, const struct RTField *field);

void RT_parser_tokenize(struct RTContext *C, struct ListBase *lb, const struct RTFile *file);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_PARSER_H
