#ifndef RT_AST_NODE_H
#define RT_AST_NODE_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RTContext;
struct RTNode;
struct RTField;
struct RTObject;
struct RTScope;
struct RTToken;
struct RTType;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RTNode {
	struct RTNode *prev, *next;

	struct {
		unsigned int kind : 16;
		unsigned int type : 16;
	};

	const struct RTToken *token;
	const struct RTType *cast;
} RTNode;

/** RTNode->kind */
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

/** RTNode->type */
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

struct RTNode *RT_node_new_block(struct RTContext *, const struct RTScope *scope);
struct RTNode *RT_node_new_variable(struct RTContext *, const struct RTObject *object);
struct RTNode *RT_node_new_typedef(struct RTContext *, const struct RTObject *object);
struct RTNode *RT_node_new_function(struct RTContext *, const struct RTObject *object);
struct RTNode *RT_node_new_constant(struct RTContext *, const struct RTToken *token);
struct RTNode *RT_node_new_funcall(struct RTContext *, const struct RTNode *func);

struct RTNode *RT_node_new_binary(struct RTContext *, const struct RTToken *token, int type, const struct RTNode *lhs, const struct RTNode *rhs);
struct RTNode *RT_node_new_cast(struct RTContext *, const struct RTToken *token, const struct RTType *type, const struct RTNode *expr);
struct RTNode *RT_node_new_unary(struct RTContext *, const struct RTToken *token, int type, const struct RTNode *expr);

struct RTNode *RT_node_new_conditional(struct RTContext *, const struct RTNode *cond, const struct RTNode *then, const struct RTNode *otherwise);
struct RTNode *RT_node_new_member(struct RTContext *, const struct RTNode *owner, const struct RTField *field);

struct RTNode *RT_node_new_constant_size(struct RTContext *, unsigned long long size);
struct RTNode *RT_node_new_constant_value(struct RTContext *, long long value);

/* -------------------------------------------------------------------- */
/** \name Block Nodes
 * \{ */

void RT_node_block_add(struct RTNode *block, const struct RTNode *node);

const struct RTNode *RT_node_block_first(const struct RTNode *node);
const struct RTNode *RT_node_block_last(const struct RTNode *node);
const struct RTScope *RT_node_block_scope(const struct RTNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Nodes
 * \{ */

const struct RTObject *RT_node_object(const struct RTNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Constant Nodes
 * \{ */

bool RT_node_is_integer(const struct RTNode *node);
bool RT_node_is_floating(const struct RTNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Binary Nodes
 * \{ */

const struct RTNode *RT_node_lhs(const struct RTNode *node);
const struct RTNode *RT_node_rhs(const struct RTNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Unary Nodes
 * \{ */

const struct RTNode *RT_node_expr(const struct RTNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Member Nodes
 * \{ */

bool RT_node_is_bitfield(const struct RTNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Funcall Nodes
 * \{ */

void RT_node_funcall_add(struct RTNode *funcall, const struct RTNode *argument);

const struct RTNode *RT_node_funcall_first(const struct RTNode *node);
const struct RTNode *RT_node_funcall_last(const struct RTNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Conditional Nodes
 * \{ */

const struct RTNode *RT_node_condition(const struct RTNode *node);
const struct RTNode *RT_node_then(const struct RTNode *node);
const struct RTNode *RT_node_otherwise(const struct RTNode *node);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_node_is_constexpr(const struct RTNode *expr);

long double RT_node_evaluate_scalar(const struct RTNode *expr);
long long RT_node_evaluate_integer(const struct RTNode *expr);

ROSE_INLINE bool RT_node_is_binary(const struct RTNode *expr);
ROSE_INLINE bool RT_node_is_block(const struct RTNode *expr);
ROSE_INLINE bool RT_node_is_conditional(const struct RTNode *expr);
ROSE_INLINE bool RT_node_is_constant(const struct RTNode *expr);
ROSE_INLINE bool RT_node_is_unary(const struct RTNode *expr);
ROSE_INLINE bool RT_node_is_object(const struct RTNode *expr);
ROSE_INLINE bool RT_node_is_member(const struct RTNode *expr);

/** \} */

#ifdef __cplusplus
}
#endif

#include "node/base_inline.c"

#endif	// RT_AST_NODE_H
