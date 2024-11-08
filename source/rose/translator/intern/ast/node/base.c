#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

void *RT_node_new(RCContext *C, const RCCToken *token, const RCCType *cast, int kind, int type, size_t length) {
	RCCNode *base = RT_context_calloc(C, length);

	if (base) {
		base->kind = kind;
		base->type = type;
		base->token = token;
		base->cast = cast;
	}

	return base;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_node_is_constexpr(const RCCNode *node) {
	switch (node->kind) {
		case NODE_CONSTANT: {
			/** Nothing to do here, constants are always constexpr. */
			return true;
		} break;
		case NODE_BINARY: {
			const struct RCCNode *lhs = RT_node_lhs(node);
			const struct RCCNode *rhs = RT_node_rhs(node);

			if (ELEM(node->type, BINARY_ASSIGN, BINARY_COMMA)) {
				return RT_node_is_constexpr(rhs);
			}

			return RT_node_is_constexpr(lhs) && RT_node_is_constexpr(rhs);
		} break;
		case NODE_UNARY: {
			const struct RCCNode *expr = RT_node_expr(node);

			if (ELEM(node->type, UNARY_NEG, UNARY_NOT, UNARY_BITNOT, UNARY_CAST)) {
				return RT_node_is_constexpr(expr);
			}
		} break;
		case NODE_COND: {
			const struct RCCNode *condition = RT_node_condition(node);
			const struct RCCNode *then = RT_node_then(node);
			const struct RCCNode *otherwise = RT_node_otherwise(node);

			if (RT_node_is_constexpr(condition)) {
				return RT_node_is_constexpr(then) && RT_node_is_constexpr(otherwise);
			}
		} break;
	}
	return false;
}

long long RT_node_evaluate_as_integer(const struct RCCNode *node) {
	switch (node->kind) {
		case NODE_CONSTANT: {
			return RT_token_as_integer(node->token);
		} break;
		default: {
			// TODO: Fix Me!
			ROSE_assert_unreachable();
		} break;
	}
	return 0;
}

/** \} */
