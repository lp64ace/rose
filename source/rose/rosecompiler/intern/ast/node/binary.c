#include "LIB_listbase.h"

#include "base.h"
#include "context.h"
#include "token.h"

typedef struct RCCNodeBinary {
	RCCNode base;
	
	const struct RCCNode *lhs;
	const struct RCCNode *rhs;
} RCCNodeBinary;

/* -------------------------------------------------------------------- */
/** \name Create Binary Node
 * \{ */

RCCNode *RCC_node_new_binary(RCContext *C, const RCCToken *token, int type, const RCCNode *lhs, const RCCNode *rhs) {
	RCCNodeBinary *node = RCC_node_new(C, token, NODE_BINARY, type, sizeof(RCCNodeBinary));
	
	node->lhs = lhs;
	node->rhs = rhs;
	
	return (RCCNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Binary Node Utils
 * \{ */

const struct RCCNode *RCC_node_lhs(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_BINARY);
	
	return ((struct RCCNodeBinary *)node)->lhs;
}
const struct RCCNode *RCC_node_rhs(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_BINARY);
	
	return ((struct RCCNodeBinary *)node)->rhs;
}

/** \} */
