#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RCCNodeBinary {
	RCCNode base;

	const struct RCCNode *lhs;
	const struct RCCNode *rhs;
} RCCNodeBinary;

/* -------------------------------------------------------------------- */
/** \name Create Binary Node
 * \{ */

RCCNode *RT_node_new_binary(RCContext *C, const RCCToken *token, int type, const RCCNode *lhs, const RCCNode *rhs) {
	const RCCType *cast = RT_type_composite(C, lhs->cast, rhs->cast);
	RCCNodeBinary *node = RT_node_new(C, token, cast, NODE_BINARY, type, sizeof(RCCNodeBinary));

	node->lhs = lhs;
	node->rhs = rhs;

	return (RCCNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Binary Node Utils
 * \{ */

const struct RCCNode *RT_node_lhs(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_BINARY);

	return ((const struct RCCNodeBinary *)node)->lhs;
}
const struct RCCNode *RT_node_rhs(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_BINARY);

	return ((const struct RCCNodeBinary *)node)->rhs;
}

/** \} */
