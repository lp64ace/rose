#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RTNodeUnary {
	RTNode base;

	const struct RTNode *expr;
} RTNodeUnary;

/* -------------------------------------------------------------------- */
/** \name Create Unary Node
 * \{ */

RTNode *RT_node_new_unary(RTContext *C, const RTToken *token, int type, const RTNode *expr) {
	RTNodeUnary *node = RT_node_new(C, token, expr->cast, NODE_UNARY, type, sizeof(RTNodeUnary));

	node->expr = expr;

	return (RTNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Unary Node Utils
 * \{ */

const struct RTNode *RT_node_expr(const struct RTNode *node) {
	ROSE_assert(node->kind == NODE_UNARY);

	return ((struct RTNodeUnary *)node)->expr;
}

/** \} */
