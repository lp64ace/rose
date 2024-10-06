#include "base.h"
#include "context.h"
#include "token.h"

/* -------------------------------------------------------------------- */
/** \name Create Constant Node
 * \{ */

RCCNode *RCC_node_new_constant(RCContext *C, const RCCToken *token) {
	RCCNode *node = RCC_node_new(C, token, NODE_CONSTANT, 0, sizeof(RCCNode));
	
	return node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Constant Node Utils
 * \{ */

bool RCC_node_is_integer(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_CONSTANT);
	
	return RCC_token_is_integer(node->token);
}
bool RCC_node_is_floating(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_CONSTANT);
	
	return RCC_token_is_floating(node->token);
}

/** \} */
