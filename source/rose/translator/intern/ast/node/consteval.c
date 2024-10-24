#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

#include "ast/node.h"
#include "ast/type.h"

/* -------------------------------------------------------------------- */
/** \name Evaluate Constant Node
 * \{ */

ROSE_INLINE bool isscalar(const RCCNode *expr) {
	if (expr->type) {
		return ELEM(expr->cast->kind, TP_FLOAT, TP_DOUBLE, TP_LDOUBLE);
	}
	return false;
}
ROSE_INLINE bool isint(const RCCNode *expr) {
	return ELEM(expr->cast->kind, TP_BOOL, TP_DOUBLE, TP_CHAR, TP_SHORT, TP_INT, TP_LONG, TP_LLONG, TP_PTR);
}

#define MANAGE_BINARY(cast, left, right, op)                                                  \
	{                                                                                         \
		if (isint(left) && isint(right)) {                                                    \
			return (cast)(RT_node_evaluate_integer(left) op RT_node_evaluate_integer(right)); \
		}                                                                                     \
		if (isint(left) && isscalar(right)) {                                                 \
			return (cast)(RT_node_evaluate_integer(left) op RT_node_evaluate_scalar(right));  \
		}                                                                                     \
		if (isscalar(left) && isint(right)) {                                                 \
			return (cast)(RT_node_evaluate_scalar(left) op RT_node_evaluate_integer(right));  \
		}                                                                                     \
		if (isscalar(left) && isscalar(right)) {                                              \
			return (cast)(RT_node_evaluate_scalar(left) op RT_node_evaluate_scalar(right));   \
		}                                                                                     \
	}
#define MANAGE_UNARY(cast, expr, op)                          \
	{                                                         \
		if (isint(expr)) {                                    \
			return (cast)(op RT_node_evaluate_integer(expr)); \
		}                                                     \
		if (isscalar(expr)) {                                 \
			return (cast)(op RT_node_evaluate_scalar(expr));  \
		}                                                     \
	}

long double RT_node_evaluate_scalar(const RCCNode *expr) {
	if(isscalar(expr)) {
		if(ELEM(expr->kind, NODE_CONSTANT)) {
			/** This is a simple constant token node that evaluates to scalar. */
			return RT_token_as_scalar(expr->token);
		}
		if(ELEM(expr->kind, NODE_BINARY)) {
			const RCCNode *lhs = RT_node_lhs(expr);
			const RCCNode *rhs = RT_node_rhs(expr);
			
			switch(expr->type) {
				case BINARY_ADD: {
					MANAGE_BINARY(long double, lhs, rhs, +);
				} break;
				case BINARY_SUB: {
					MANAGE_BINARY(long double, lhs, rhs, -);
				} break;
				case BINARY_MUL: {
					MANAGE_BINARY(long double, lhs, rhs, *);
				} break;
				case BINARY_DIV: {
					MANAGE_BINARY(long double, lhs, rhs, /);
				} break;
				case BINARY_AND: {
					MANAGE_BINARY(long double, lhs, rhs, &&);
				} break;
				case BINARY_OR: {
					MANAGE_BINARY(long double, lhs, rhs, ||);
				} break;
				case BINARY_EQUAL: {
					MANAGE_BINARY(long double, lhs, rhs, ==);
				} break;
				case BINARY_NEQUAL: {
					MANAGE_BINARY(long double, lhs, rhs, !=);
				} break;
				case BINARY_LEQUAL: {
					MANAGE_BINARY(long double, lhs, rhs, <=);
				} break;
				case BINARY_LESS: {
					MANAGE_BINARY(long double, lhs, rhs, <);
				} break;
				
				case BINARY_ASSIGN:
				case BINARY_COMMA: {
					return RT_node_evaluate_scalar(rhs);
				} break;
			}
		}
		if(ELEM(expr->kind, NODE_UNARY)) {
			const RCCNode *node = RT_node_expr(expr);
			
			switch(expr->type) {
				case UNARY_RETURN: {
					MANAGE_UNARY(long double, node, -);
				} break;
				case UNARY_NEG: {
					MANAGE_UNARY(long double, node, -);
				} break;
				case UNARY_NOT: {
					MANAGE_UNARY(long double, node, !);
				} break;
			}
		}
		if(ELEM(expr->kind, NODE_COND)) {
			const RCCNode *condition = RT_node_condition(expr);
			const RCCNode *then = RT_node_then(expr);
			const RCCNode *otherwise = RT_node_otherwise(expr);
			
			if(isint(condition)) {
				bool path = RT_node_evaluate_integer(condition);
				
				return path ? RT_node_evaluate_scalar(then) : RT_node_evaluate_scalar(otherwise);
			}
			else if(isscalar(condition)) {
				bool path = RT_node_evaluate_scalar(condition);
				
				return path ? RT_node_evaluate_scalar(then) : RT_node_evaluate_scalar(otherwise);
			}
		}
		ROSE_assert_unreachable();
		return 0;
	}
	return (long double)(isint(expr) ? RT_node_evaluate_integer(expr) : 0);
}
long long RT_node_evaluate_integer(const RCCNode *expr) {
	if(isint(expr)) {
		if(ELEM(expr->kind, NODE_CONSTANT)) {
			/** This is a simple constant token node that evaluates to integer. */
			return (long long)RT_token_as_integer(expr->token);
		}
		if(ELEM(expr->kind, NODE_BINARY)) {
			const RCCNode *lhs = RT_node_lhs(expr);
			const RCCNode *rhs = RT_node_rhs(expr);
			
			switch(expr->type) {
				case BINARY_ADD: {
					MANAGE_BINARY(long long, lhs, rhs, +);
				} break;
				case BINARY_SUB: {
					MANAGE_BINARY(long long, lhs, rhs, -);
				} break;
				case BINARY_MUL: {
					MANAGE_BINARY(long long, lhs, rhs, *);
				} break;
				case BINARY_DIV: {
					MANAGE_BINARY(long long, lhs, rhs, /);
				} break;
				case BINARY_MOD: {
					return RT_node_evaluate_integer(lhs) % RT_node_evaluate_integer(rhs);
				} break;
				case BINARY_AND: {
					MANAGE_BINARY(long long, lhs, rhs, &&);
				} break;
				case BINARY_OR: {
					MANAGE_BINARY(long long, lhs, rhs, ||);
				} break;
				case BINARY_BITAND: {
					return RT_node_evaluate_integer(lhs) & RT_node_evaluate_integer(rhs);
				} break;
				case BINARY_BITOR: {
					return RT_node_evaluate_integer(lhs) | RT_node_evaluate_integer(rhs);
				} break;
				case BINARY_BITXOR: {
					return RT_node_evaluate_integer(lhs) ^ RT_node_evaluate_integer(rhs);
				} break;
				case BINARY_LSHIFT: {
					return RT_node_evaluate_integer(lhs) << RT_node_evaluate_integer(rhs);
				} break;
				case BINARY_RSHIFT: {
					return RT_node_evaluate_integer(lhs) >> RT_node_evaluate_integer(rhs);
				} break;
				case BINARY_EQUAL: {
					MANAGE_BINARY(long long, lhs, rhs, ==);
				} break;
				case BINARY_NEQUAL: {
					MANAGE_BINARY(long long, lhs, rhs, !=);
				} break;
				case BINARY_LEQUAL: {
					MANAGE_BINARY(long long, lhs, rhs, <=);
				} break;
				case BINARY_LESS: {
					MANAGE_BINARY(long long, lhs, rhs, <);
				} break;
				
				case BINARY_ASSIGN:
				case BINARY_COMMA: {
					return RT_node_evaluate_integer(rhs);
				} break;
			}
		}
		if(ELEM(expr->kind, NODE_UNARY)) {
			const RCCNode *node = RT_node_expr(expr);
			
			switch(expr->type) {
				case UNARY_RETURN: {
					MANAGE_UNARY(long double, node, -);
				} break;
				case UNARY_NEG: {
					MANAGE_UNARY(long long, node, -);
				} break;
				case UNARY_NOT: {
					MANAGE_UNARY(long long, node, !);
				} break;
			}
		}
		if(ELEM(expr->kind, NODE_COND)) {
			const RCCNode *condition = RT_node_condition(expr);
			const RCCNode *then = RT_node_then(expr);
			const RCCNode *otherwise = RT_node_otherwise(expr);
			
			if(isint(condition)) {
				bool path = RT_node_evaluate_integer(condition);
				
				return path ? RT_node_evaluate_scalar(then) : RT_node_evaluate_scalar(otherwise);
			}
			else if(isscalar(condition)) {
				bool path = RT_node_evaluate_scalar(condition);
				
				return path ? RT_node_evaluate_scalar(then) : RT_node_evaluate_scalar(otherwise);
			}
		}
		ROSE_assert_unreachable();
		return 0;
	}
	return (long long)(isscalar(expr) ? RT_node_evaluate_scalar(expr) : 0);
}

/** \} */
