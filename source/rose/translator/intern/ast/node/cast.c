#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

/* -------------------------------------------------------------------- */
/** \name Cast Nodes
 * \{ */

RCCNode *RT_node_new_cast(RCContext *C, const RCCToken *token, const RCCType *type, const RCCNode *expr) {
	RCCNode *node = RT_node_new_unary(C, token, UNARY_CAST, expr);

	node->cast = type;

	return node;
}

/** \} */
