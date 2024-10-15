#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

/* -------------------------------------------------------------------- */
/** \name Create Constant Node
 * \{ */

RCCNode *RT_node_new_constant(RCContext *C, const RCCToken *token) {
	RCCNode *node = RT_node_new(C, token, token->type, NODE_CONSTANT, 0, sizeof(RCCNode));
	
	return node;
}

RCCNode *RT_node_new_constant_size(RCContext *C, unsigned long long size) {
	RCCToken *token = RT_token_new_size(C, size);
	
	return RT_node_new_constant(C, token);
}
RCCNode *RT_node_new_constant_value(RCContext *C, long long value) {
	RCCToken *token = RT_token_new_llong(C, value);
	
	return RT_node_new_constant(C, token);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Constant Node Utils
 * \{ */

bool RT_node_is_integer(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_CONSTANT);
	
	return RT_token_is_integer(node->token);
}
bool RT_node_is_floating(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_CONSTANT);
	
	return RT_token_is_floating(node->token);
}

/** \} */
