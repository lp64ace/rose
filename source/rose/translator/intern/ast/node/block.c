#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RCCNodeBlock {
	RCCNode base;
	
	const struct RCCScope *scope;
	
	ListBase nodes;
} RCCNodeBlock;

/* -------------------------------------------------------------------- */
/** \name Block Nodes
 * \{ */

RCCNode *RT_node_new_block(RCContext *C, const struct RCCScope *scope) {
	RCCNodeBlock *node = RT_node_new(C, NULL, NULL, NODE_BLOCK, 0, sizeof(RCCNodeBlock));
	
	node->scope = scope;
	
	return (RCCNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Block Node Utils
 * \{ */

void RT_node_block_add(RCCNode *node, const RCCNode *expr) {
	RCCNodeBlock *block = (RCCNodeBlock *)node;
	
	LIB_addtail(&block->nodes, (void *)expr);
}
const RCCNode *RT_node_block_first(const RCCNode *node) {
	ROSE_assert(node->kind == NODE_BLOCK);
	
	return (const RCCNode *)(((const RCCNodeBlock *)node)->nodes.first);
}
const RCCNode *RT_node_block_last(const RCCNode *node) {
	ROSE_assert(node->kind == NODE_BLOCK);
	
	return (const RCCNode *)(((const RCCNodeBlock *)node)->nodes.last);
}
const struct RCCScope *RT_node_block_scope(const RCCNode *node) {
	ROSE_assert(node->kind == NODE_BLOCK);
	
	return ((const RCCNodeBlock *)node)->scope;
}

/** \} */
