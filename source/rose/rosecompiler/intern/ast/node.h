#ifndef __RCC_AST_NODE_H__
#define __RCC_AST_NODE_H__

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;
struct RCCNode;
struct RCCToken;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCNode {
	struct RCCNode *prev, *next;
	
	struct {
		int kind : 16;
		int type : 16;
	};
	
	const struct RCCToken *token;
} RCCNode;

/** RCCNode->kind */
enum {
	NODE_CONSTANT,
	NODE_LITERAL,
	NODE_BINARY,
	NODE_UNARY,
	NODE_DECLARATION,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Constant Nodes
 * \{ */

struct RCCNode *RCC_node_new_constant(struct RCContext *, const struct RCCToken *token);

bool RCC_node_is_integer(const struct RCCNode *node);
bool RCC_node_is_floating(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Binary Nodes
 * \{ */

/** RCCNode->type */
enum {
	BINARY_ADD,
	BINARY_SUB,
	BINARY_MUL,
	BINARY_DIV,
	BINARY_MOD,
	BINARY_BITAND,
	BINARY_BITOR,
	BINARY_BITXOR,
	BINARY_LSHIFT,
	BINARY_RSHIFT,
	BINARY_EQUAL,
	BINARY_NEQUAL,
	BINARY_LEQUAL,
	BINARY_LESS,
};

struct RCCNode *RCC_node_new_binary(struct RCContext *, const struct RCCToken *token, int type, const struct RCCNode *lhs, const struct RCCNode *rhs);

const struct RCCNode *RCC_node_lhs(const struct RCCNode *node);
const struct RCCNode *RCC_node_rhs(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Unary Nodes
 * \{ */

/** RCCNode->type */
enum {
	UNARY_NEG,
	UNARY_NOT,
	UNARY_BITNOT,
	UNARY_ADDR,
	UNARY_DEREF,
};

struct RCCNode *RCC_node_new_unary(struct RCContext *, const struct RCCToken *token, int type, const struct RCCNode *expr);

const struct RCCNode *RCC_node_expr(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Declaration Nodes
 * \{ */

struct RCCNode *RCC_node_new_declaration(struct RCContext *, const struct RCCToken *token, const struct RCCType *type, const struct RCCObject *object);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RCC_node_is_constexpr(const struct RCCNode *node);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // __RCC_AST_NODE_H__
