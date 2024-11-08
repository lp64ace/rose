#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RCCNodeUnary {
	RCCNode base;

	const struct RCCNode *expr;
} RCCNodeUnary;

/* -------------------------------------------------------------------- */
/** \name Create Unary Node
 * \{ */

RCCNode *RT_node_new_unary(RCContext *C, const RCCToken *token, int type, const RCCNode *expr) {
	RCCNodeUnary *node = RT_node_new(C, token, expr->cast, NODE_UNARY, type, sizeof(RCCNodeUnary));

	node->expr = expr;

	return (RCCNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Unary Node Utils
 * \{ */

const struct RCCNode *RT_node_expr(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_UNARY);

	return ((struct RCCNodeUnary *)node)->expr;
}

/** \} */
