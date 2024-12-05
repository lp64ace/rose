#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

/* -------------------------------------------------------------------- */
/** \name Cast Nodes
 * \{ */

RTNode *RT_node_new_cast(RTContext *C, const RTToken *token, const RTType *type, const RTNode *expr) {
	RTNode *node = RT_node_new_unary(C, token, UNARY_CAST, expr);

	node->cast = type;

	return node;
}

/** \} */
