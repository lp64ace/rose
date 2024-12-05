#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

void *RT_node_new(RTContext *C, const RTToken *token, const RTType *cast, int kind, int type, size_t length) {
	RTNode *base = RT_context_calloc(C, length);

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

bool RT_node_is_constexpr(const RTNode *node) {
	switch (node->kind) {
		case NODE_CONSTANT: {
			/** Nothing to do here, constants are always constexpr. */
			return true;
		} break;
		case NODE_BINARY: {
			const struct RTNode *lhs = RT_node_lhs(node);
			const struct RTNode *rhs = RT_node_rhs(node);

			if (ELEM(node->type, BINARY_ASSIGN, BINARY_COMMA)) {
				return RT_node_is_constexpr(rhs);
			}

			return RT_node_is_constexpr(lhs) && RT_node_is_constexpr(rhs);
		} break;
		case NODE_UNARY: {
			const struct RTNode *expr = RT_node_expr(node);

			if (ELEM(node->type, UNARY_NEG, UNARY_NOT, UNARY_BITNOT, UNARY_CAST)) {
				return RT_node_is_constexpr(expr);
			}
		} break;
		case NODE_COND: {
			const struct RTNode *condition = RT_node_condition(node);
			const struct RTNode *then = RT_node_then(node);
			const struct RTNode *otherwise = RT_node_otherwise(node);

			if (RT_node_is_constexpr(condition)) {
				return RT_node_is_constexpr(then) && RT_node_is_constexpr(otherwise);
			}
		} break;
	}
	return false;
}

long long RT_node_evaluate_as_integer(const struct RTNode *node) {
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
