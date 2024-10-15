#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RCCNodeConditional {
	RCCNode base;
	
	const struct RCCNode *condition;
	const struct RCCNode *then;
	const struct RCCNode *otherwise;
} RCCNodeConditional;

/* -------------------------------------------------------------------- */
/** \name Conditional Nodes
 * \{ */

RCCNode *RT_node_new_conditional(RCContext *C, const RCCNode *condition, const RCCNode *then, const RCCNode *otherwise) {
	const RCCType *cast = RT_type_composite(C, then->cast, otherwise->cast);
	RCCNodeConditional *node = RT_node_new(C, NULL, cast, NODE_COND, 0, sizeof(RCCNodeConditional));
	
	node->condition = condition;
	node->then = then;
	node->otherwise = otherwise;
	
	return (RCCNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conditional Node Utils
 * \{ */

const struct RCCNode *RT_node_condition(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_COND);
	
	return ((const RCCNodeConditional *)node)->condition;
}
const struct RCCNode *RT_node_then(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_COND);
	
	return ((const RCCNodeConditional *)node)->then;
}
const struct RCCNode *RT_node_otherwise(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_COND);
	
	return ((const RCCNodeConditional *)node)->otherwise;
}

/** \} */
