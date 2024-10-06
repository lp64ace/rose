#include "base.h"

#include "context.h"
#include "token.h"

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

void *RCC_node_new(RCContext *C, const RCCToken *token, int kind, int type, size_t length) {
	RCCNode *base = RCC_context_calloc(C, length);
	
	if(base) {
		base->kind = kind;
		base->type = type;
		base->token = token;
	}
	
	return base;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RCC_node_is_constexpr(const RCCNode *node) {
	switch(node->kind) {
		case NODE_CONSTANT: {
			/** Nothing to do here, constants are always constexpr. */
		} return true;
		case NODE_BINARY: {
			const struct RCCNode *lhs = RCC_node_lhs(node);
			const struct RCCNode *rhs = RCC_node_rhs(node);
			
			return RCC_node_is_constexpr(lhs) && RCC_node_is_constexpr(rhs);
		} break;
		case NODE_UNARY: {
			const struct RCCNode *expr = RCC_node_expr(node);
			
			if(node->type == UNARY_ADDR) {
				return true;
			}
			return RCC_node_is_constexpr(expr);
		} break;
	}
	return false;
}

/** \} */
