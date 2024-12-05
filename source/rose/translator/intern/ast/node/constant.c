#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

/* -------------------------------------------------------------------- */
/** \name Create Constant Node
 * \{ */

RTNode *RT_node_new_constant(RTContext *C, const RTToken *token) {
	RTNode *node = RT_node_new(C, token, token->type, NODE_CONSTANT, 0, sizeof(RTNode));

	return node;
}

RTNode *RT_node_new_constant_size(RTContext *C, unsigned long long size) {
	RTToken *token = RT_token_new_virtual_size(C, size);

	return RT_node_new_constant(C, token);
}
RTNode *RT_node_new_constant_value(RTContext *C, long long value) {
	RTToken *token = RT_token_new_virtual_llong(C, value);

	return RT_node_new_constant(C, token);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Constant Node Utils
 * \{ */

bool RT_node_is_integer(const struct RTNode *node) {
	ROSE_assert(node->kind == NODE_CONSTANT);

	return RT_token_is_integer(node->token);
}
bool RT_node_is_floating(const struct RTNode *node) {
	ROSE_assert(node->kind == NODE_CONSTANT);

	return RT_token_is_floating(node->token);
}

/** \} */
