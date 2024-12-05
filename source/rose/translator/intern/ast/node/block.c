#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RTNodeBlock {
	RTNode base;

	const struct RTScope *scope;

	ListBase nodes;
} RTNodeBlock;

/* -------------------------------------------------------------------- */
/** \name Block Nodes
 * \{ */

RTNode *RT_node_new_block(RTContext *C, const struct RTScope *scope) {
	RTNodeBlock *node = RT_node_new(C, NULL, NULL, NODE_BLOCK, 0, sizeof(RTNodeBlock));

	node->scope = scope;

	return (RTNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Block Node Utils
 * \{ */

void RT_node_block_add(RTNode *node, const RTNode *expr) {
	RTNodeBlock *block = (RTNodeBlock *)node;

	LIB_addtail(&block->nodes, (void *)expr);
}
const RTNode *RT_node_block_first(const RTNode *node) {
	ROSE_assert(node->kind == NODE_BLOCK);

	return (const RTNode *)(((const RTNodeBlock *)node)->nodes.first);
}
const RTNode *RT_node_block_last(const RTNode *node) {
	ROSE_assert(node->kind == NODE_BLOCK);

	return (const RTNode *)(((const RTNodeBlock *)node)->nodes.last);
}
const struct RTScope *RT_node_block_scope(const RTNode *node) {
	ROSE_assert(node->kind == NODE_BLOCK);

	return ((const RTNodeBlock *)node)->scope;
}

/** \} */
