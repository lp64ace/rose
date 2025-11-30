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

typedef struct RTCParser {
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

struct RTCParser *RT_parser_new(const struct RTFile *file);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RT_parser_free(struct RTCParser *parser);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

struct RTContext *RT_parser_context(struct RTCParser *parser);

bool RT_parser_do(struct RTCParser *parser);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

const struct RTNode *RT_parser_conditional(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);

const struct RTType *RT_parser_struct(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTType *RT_parser_enum(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);

const struct RTType *RT_parser_declspec(struct RTCParser *P, struct RTToken **rest, struct RTToken *token, DeclInfo *info);
const struct RTType *RT_parser_typename(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTType *RT_parser_pointers(struct RTCParser *P, struct RTToken **rest, struct RTToken *token, const RTType *source);
const struct RTType *RT_parser_abstract(struct RTCParser *P, struct RTToken **rest, struct RTToken *token, const RTType *source);
const struct RTType *RT_parser_funcparams(struct RTCParser *P, struct RTToken **rest, struct RTToken *token, const RTType *result);
const struct RTType *RT_parser_dimensions(struct RTCParser *P, struct RTToken **rest, struct RTToken *token, const RTType *element);

struct RTObject *RT_parser_declarator(struct RTCParser *P, struct RTToken **rest, struct RTToken *token, const struct RTType *source);

const struct RTNode *RT_parser_assign(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_cast(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_unary(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_postfix(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_mul(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_add(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_shift(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_relational(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_equality(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_bitand(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_bitxor(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_bitor(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_logand(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_logor(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_expr(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);
const struct RTNode *RT_parser_stmt(struct RTCParser *P, struct RTToken **rest, struct RTToken *token);

unsigned long long RT_parser_alignof(struct RTCParser *P, const struct RTType *type);
unsigned long long RT_parser_size(struct RTCParser *P, const struct RTType *type);
unsigned long long RT_parser_offsetof(struct RTCParser *P, const struct RTType *type, const struct RTField *field);

void RT_parser_tokenize(struct RTContext *C, struct ListBase *lb, const struct RTFile *file);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_PARSER_H
