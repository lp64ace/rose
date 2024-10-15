#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RCCNodeFuncall {
	RCCNode base;
	
	const RCCNode *func;
	
	ListBase arguments;
} RCCNodeFuncall;

/* -------------------------------------------------------------------- */
/** \name Funcall Nodes
 * \{ */

RCCNode *RT_node_new_funcall(RCContext *C, const RCCNode *func) {
	RCCNodeFuncall *node = RT_node_new(C, func->token, func->cast, NODE_FUNCALL, 0, sizeof(RCCNodeFuncall));
	
	node->func = func;
	
	return (RCCNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Funcall Node Utils
 * \{ */

void RT_node_funcall_add(RCCNode *funcall, const RCCNode *argument) {
	RCCNodeFuncall *node = (RCCNodeFuncall *)funcall;

	LIB_addtail(&node->arguments, (void *)argument);
}

const RCCNode *RT_node_funcall_first(const RCCNode *node) {
	ROSE_assert(node->kind == NODE_FUNCALL);

	return (const RCCNode *)(((const RCCNodeFuncall *)node)->arguments.first);
}
const RCCNode *RT_node_funcall_last(const RCCNode *node) {
	ROSE_assert(node->kind == NODE_FUNCALL);

	return (const RCCNode *)(((const RCCNodeFuncall *)node)->arguments.last);
}

/** \} */
