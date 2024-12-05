#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RTNodeBinary {
	RTNode base;

	const struct RTNode *lhs;
	const struct RTNode *rhs;
} RTNodeBinary;

/* -------------------------------------------------------------------- */
/** \name Create Binary Node
 * \{ */

RTNode *RT_node_new_binary(RTContext *C, const RTToken *token, int type, const RTNode *lhs, const RTNode *rhs) {
	const RTType *cast = RT_type_composite(C, lhs->cast, rhs->cast);
	RTNodeBinary *node = RT_node_new(C, token, cast, NODE_BINARY, type, sizeof(RTNodeBinary));

	node->lhs = lhs;
	node->rhs = rhs;

	return (RTNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Binary Node Utils
 * \{ */

const struct RTNode *RT_node_lhs(const struct RTNode *node) {
	ROSE_assert(node->kind == NODE_BINARY);

	return ((const struct RTNodeBinary *)node)->lhs;
}
const struct RTNode *RT_node_rhs(const struct RTNode *node) {
	ROSE_assert(node->kind == NODE_BINARY);

	return ((const struct RTNodeBinary *)node)->rhs;
}

/** \} */
