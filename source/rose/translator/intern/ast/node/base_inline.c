#ifndef RT_AST_NODE_BASE_INLINE_C
#define RT_AST_NODE_BASE_INLINE_C

#include "../node.h"

#ifdef __cplusplus
extern "C" {
#endif

ROSE_INLINE bool RT_node_is_binary(const struct RCCNode *expr) {
	return ELEM(expr->kind, NODE_BINARY);
}
ROSE_INLINE bool RT_node_is_block(const struct RCCNode *expr) {
	return ELEM(expr->kind, NODE_BLOCK);
}
ROSE_INLINE bool RT_node_is_conditional(const struct RCCNode *expr) {
	return ELEM(expr->kind, NODE_COND);
}
ROSE_INLINE bool RT_node_is_constant(const struct RCCNode *expr) {
	return ELEM(expr->kind, NODE_CONSTANT);
}
ROSE_INLINE bool RT_node_is_unary(const struct RCCNode *expr) {
	return ELEM(expr->kind, NODE_UNARY);
}
ROSE_INLINE bool RT_node_is_object(const struct RCCNode *expr) {
	return ELEM(expr->kind, NODE_OBJECT);
}
ROSE_INLINE bool RT_node_is_member(const struct RCCNode *expr) {
	return ELEM(expr->kind, NODE_MEMBER);
}

#ifdef __cplusplus
}
#endif

#endif // RT_AST_NODE_BASE_INLINE_C
