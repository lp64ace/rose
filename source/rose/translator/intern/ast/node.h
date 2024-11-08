#ifndef RT_AST_NODE_H
#define RT_AST_NODE_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;
struct RCCNode;
struct RCCField;
struct RCCObject;
struct RCCScope;
struct RCCToken;
struct RCCType;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCNode {
	struct RCCNode *prev, *next;

	struct {
		unsigned int kind : 16;
		unsigned int type : 16;
	};

	const struct RCCToken *token;
	const struct RCCType *cast;
} RCCNode;

/** RCCNode->kind */
enum {
	NODE_BINARY,
	NODE_BLOCK,
	NODE_COND,
	NODE_CONSTANT,
	NODE_FUNCALL,
	NODE_MEMBER,
	NODE_OBJECT,
	NODE_UNARY,
};

/** RCCNode->type */
enum {
	BINARY_ADD,
	BINARY_SUB,
	BINARY_MUL,
	BINARY_DIV,
	BINARY_MOD,
	BINARY_AND,
	BINARY_OR,
	BINARY_BITAND,
	BINARY_BITOR,
	BINARY_BITXOR,
	BINARY_LSHIFT,
	BINARY_RSHIFT,
	BINARY_EQUAL,
	BINARY_NEQUAL,
	BINARY_LEQUAL,
	BINARY_LESS,
	BINARY_ASSIGN,
	BINARY_COMMA,

	UNARY_NEG,
	UNARY_NOT,
	UNARY_BITNOT,
	UNARY_CAST,
	UNARY_FUNCALL,
	UNARY_RETURN,
	UNARY_ADDR,
	UNARY_DEREF,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCNode *RT_node_new_block(struct RCContext *, const struct RCCScope *scope);
struct RCCNode *RT_node_new_variable(struct RCContext *, const struct RCCObject *object);
struct RCCNode *RT_node_new_typedef(struct RCContext *, const struct RCCObject *object);
struct RCCNode *RT_node_new_function(struct RCContext *, const struct RCCObject *object);
struct RCCNode *RT_node_new_constant(struct RCContext *, const struct RCCToken *token);
struct RCCNode *RT_node_new_funcall(struct RCContext *, const struct RCCNode *func);

struct RCCNode *RT_node_new_binary(struct RCContext *, const struct RCCToken *token, int type, const struct RCCNode *lhs, const struct RCCNode *rhs);
struct RCCNode *RT_node_new_cast(struct RCContext *, const struct RCCToken *token, const struct RCCType *type, const struct RCCNode *expr);
struct RCCNode *RT_node_new_unary(struct RCContext *, const struct RCCToken *token, int type, const struct RCCNode *expr);

struct RCCNode *RT_node_new_conditional(struct RCContext *, const struct RCCNode *cond, const struct RCCNode *then, const struct RCCNode *otherwise);
struct RCCNode *RT_node_new_member(struct RCContext *, const struct RCCNode *owner, const struct RCCField *field);

struct RCCNode *RT_node_new_constant_size(struct RCContext *, unsigned long long size);
struct RCCNode *RT_node_new_constant_value(struct RCContext *, long long value);

/* -------------------------------------------------------------------- */
/** \name Block Nodes
 * \{ */

void RT_node_block_add(struct RCCNode *block, const struct RCCNode *node);

const struct RCCNode *RT_node_block_first(const struct RCCNode *node);
const struct RCCNode *RT_node_block_last(const struct RCCNode *node);
const struct RCCScope *RT_node_block_scope(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Nodes
 * \{ */

const struct RCCObject *RT_node_object(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Constant Nodes
 * \{ */

bool RT_node_is_integer(const struct RCCNode *node);
bool RT_node_is_floating(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Binary Nodes
 * \{ */

const struct RCCNode *RT_node_lhs(const struct RCCNode *node);
const struct RCCNode *RT_node_rhs(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Unary Nodes
 * \{ */

const struct RCCNode *RT_node_expr(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Member Nodes
 * \{ */

bool RT_node_is_bitfield(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Funcall Nodes
 * \{ */

void RT_node_funcall_add(struct RCCNode *funcall, const struct RCCNode *argument);

const struct RCCNode *RT_node_funcall_first(const struct RCCNode *node);
const struct RCCNode *RT_node_funcall_last(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conditional Nodes
 * \{ */

const struct RCCNode *RT_node_condition(const struct RCCNode *node);
const struct RCCNode *RT_node_then(const struct RCCNode *node);
const struct RCCNode *RT_node_otherwise(const struct RCCNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_node_is_constexpr(const struct RCCNode *expr);

long double RT_node_evaluate_scalar(const struct RCCNode *expr);
long long RT_node_evaluate_integer(const struct RCCNode *expr);

ROSE_INLINE bool RT_node_is_binary(const struct RCCNode *expr);
ROSE_INLINE bool RT_node_is_block(const struct RCCNode *expr);
ROSE_INLINE bool RT_node_is_conditional(const struct RCCNode *expr);
ROSE_INLINE bool RT_node_is_constant(const struct RCCNode *expr);
ROSE_INLINE bool RT_node_is_unary(const struct RCCNode *expr);
ROSE_INLINE bool RT_node_is_object(const struct RCCNode *expr);
ROSE_INLINE bool RT_node_is_member(const struct RCCNode *expr);

/** \} */

#ifdef __cplusplus
}
#endif

#include "node/base_inline.c"

#endif	// RT_AST_NODE_H
