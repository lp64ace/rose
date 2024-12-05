#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RTNodeFuncall {
	RTNode base;

	const RTNode *func;

	ListBase arguments;
} RTNodeFuncall;

/* -------------------------------------------------------------------- */
/** \name Funcall Nodes
 * \{ */

RTNode *RT_node_new_funcall(RTContext *C, const RTNode *func) {
	RTNodeFuncall *node = RT_node_new(C, func->token, func->cast, NODE_FUNCALL, 0, sizeof(RTNodeFuncall));

	node->func = func;

	return (RTNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Funcall Node Utils
 * \{ */

void RT_node_funcall_add(RTNode *funcall, const RTNode *argument) {
	RTNodeFuncall *node = (RTNodeFuncall *)funcall;

	LIB_addtail(&node->arguments, (void *)argument);
}

const RTNode *RT_node_funcall_first(const RTNode *node) {
	ROSE_assert(node->kind == NODE_FUNCALL);

	return (const RTNode *)(((const RTNodeFuncall *)node)->arguments.first);
}
const RTNode *RT_node_funcall_last(const RTNode *node) {
	ROSE_assert(node->kind == NODE_FUNCALL);

	return (const RTNode *)(((const RTNodeFuncall *)node)->arguments.last);
}

/** \} */
