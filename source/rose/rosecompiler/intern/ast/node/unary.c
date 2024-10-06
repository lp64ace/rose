#include "base.h"
#include "context.h"
#include "token.h"

typedef struct RCCNodeUnary {
	RCCNode base;
	
	const struct RCCNode *expr;
} RCCNodeUnary;

/* -------------------------------------------------------------------- */
/** \name Create Unary Node
 * \{ */

RCCNode *RCC_node_new_unary(RCContext *C, const RCCToken *token, int type, const RCCNode *expr) {
	RCCNodeUnary *node = RCC_node_new(C, token, NODE_UNARY, type, sizeof(RCCNodeUnary));
	
	node->expr = expr;
	
	return (RCCNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Unary Node Utils
 * \{ */

const struct RCCNode *RCC_node_expr(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_UNARY);
	
	return ((struct RCCNodeUnary *)node)->expr;
}

/** \} */
