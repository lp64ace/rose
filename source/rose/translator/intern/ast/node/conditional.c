#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RTNodeConditional {
	RTNode base;

	const struct RTNode *condition;
	const struct RTNode *then;
	const struct RTNode *otherwise;
} RTNodeConditional;

/* -------------------------------------------------------------------- */
/** \name Conditional Nodes
 * \{ */

RTNode *RT_node_new_conditional(RTContext *C, const RTNode *condition, const RTNode *then, const RTNode *otherwise) {
	const RTType *cast = RT_type_composite(C, then->cast, otherwise->cast);
	RTNodeConditional *node = RT_node_new(C, NULL, cast, NODE_COND, 0, sizeof(RTNodeConditional));

	node->condition = condition;
	node->then = then;
	node->otherwise = otherwise;

	return (RTNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conditional Node Utils
 * \{ */

const struct RTNode *RT_node_condition(const struct RTNode *node) {
	ROSE_assert(node->kind == NODE_COND);

	return ((const RTNodeConditional *)node)->condition;
}
const struct RTNode *RT_node_then(const struct RTNode *node) {
	ROSE_assert(node->kind == NODE_COND);

	return ((const RTNodeConditional *)node)->then;
}
const struct RTNode *RT_node_otherwise(const struct RTNode *node) {
	ROSE_assert(node->kind == NODE_COND);

	return ((const RTNodeConditional *)node)->otherwise;
}

/** \} */
