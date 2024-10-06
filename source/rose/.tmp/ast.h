#ifndef __RCC_PARSER_H__
#define __RCC_PARSER_H__

#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCNode {
	struct RCCNode *prev, *next;
	
	int kind;
	
	RCCType *type;
	RCCToken *token;
	
	RCCNode *lhs, *rhs;
} RCCNode;

enum {
	ND_NONE,
	ND_ADD,
	ND_SUB,
	ND_MUL,
	ND_DIV,
	ND_NEG,
	ND_MOD,
	ND_BITAND,
	ND_BITOR,
	ND_BITXOR,
	ND_LSHIFT,
	ND_RSHIFT,
	ND_EQ,
	ND_NE,
	ND_LT,
	ND_LE,
	ND_ASSIGN,
	ND_COND,
	ND_COMMA,
	ND_MEMBER,
	ND_ADDR,
	ND_DEREF,
	ND_NOT,
	ND_BITNOT,
	ND_LOGAND,
	ND_LOGOR,
	ND_RETURN,
	ND_IF,
	ND_FOR,
	ND_DO,
	ND_SWITCH,
	ND_CASE,
	ND_BLOCK,
	ND_GOTO,
	ND_GOTO_EXPR,
	ND_LABEL,
	ND_LABEL_VAL,
	ND_FUNCALL,
	ND_EXPR_STMT,
	ND_STMT_EXPR,
	ND_VAR,
	ND_NUM,
	ND_CAST,
	ND_MEMZERO,
	ND_ASM,
	ND_CAS,			// Atomic compare-and-swap
	ND_EXCH,		// Atomic exchange
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

RCCNode *RCC_node_new_cast(RCCNode *expr, RCCType *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RCC_node_free(RCCNode *node);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // __RCC_PARSER_H__
