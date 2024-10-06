#ifndef __RCC_NODE_H__
#define __RCC_NODE_H__

#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCCObject;
struct RCCToken;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/** Note that this is only the tip of the iceberg, this is only the base class! */
typedef struct RCCNode {
	struct RCCNode *prev, *next;
	
	int kind;
	
	RCCType *type;
	
	struct RCCToken *token;
	struct RCCNode *lhs;
	struct RCCNode *rhs;
} RCCNode;

enum {
	ND_NULL_EXPR, // Do nothing
	ND_ADD,       // +
	ND_SUB,       // -
	ND_MUL,       // *
	ND_DIV,       // /
	ND_NEG,       // unary -
	ND_MOD,       // %
	ND_BITAND,    // &
	ND_BITOR,     // |
	ND_BITXOR,    // ^
	ND_SHL,       // <<
	ND_SHR,       // >>
	ND_EQ,        // ==
	ND_NE,        // !=
	ND_LT,        // <
	ND_LE,        // <=
	ND_ASSIGN,    // =
	ND_COND,      // ?:
	ND_COMMA,     // ,
	ND_MEMBER,    // . (struct member access)
	ND_ADDR,      // unary &
	ND_DEREF,     // unary *
	ND_NOT,       // !
	ND_BITNOT,    // ~
	ND_LOGAND,    // &&
	ND_LOGOR,     // ||
	ND_RETURN,    // "return"
	ND_IF,        // "if"
	ND_FOR,       // "for" or "while"
	ND_DO,        // "do"
	ND_SWITCH,    // "switch"
	ND_CASE,      // "case"
	ND_BLOCK,     // { ... }
	ND_GOTO,      // "goto"
	ND_GOTO_EXPR, // "goto" labels-as-values
	ND_LABEL,     // Labeled statement
	ND_LABEL_VAL, // [GNU] Labels-as-values
	ND_FUNCALL,   // Function call
	ND_EXPR_STMT, // Expression statement
	ND_STMT_EXPR, // Statement expression
	ND_VAR,       // Variable
	ND_VLA_PTR,   // VLA designator
	ND_NUM,       // Integer
	ND_CAST,      // Type cast
	ND_MEMZERO,   // Zero-clear a stack variable
	ND_ASM,       // "asm"
	ND_CAS,       // Atomic compare-and-swap
	ND_EXCH,      // Atomic exchange
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCNode *RCC_node_new(int kind, RCCToken *token);
struct RCCNode *RCC_node_new_binary(int kind, RCCToken *token, RCCNode *lhs, RCCNode *rhs);
struct RCCNode *RCC_node_new_unary(int kind, RCCToken *token, RCCNode *expr);
struct RCCNode *RCC_node_new_number(int kind, RCCToken *token);
struct RCCNode *RCC_node_new_var(int kind, RCCToken *token, RCCObject *obj);
struct RCCNode *RCC_node_new_vla_ptr(int kind, RCCToken *token, RCCObject *obj);
struct RCCNode *RCC_node_new_cast(int kind, RCCToken *token, RCCObject *obj);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

/** \} */

#ifdef __cplusplus
}
#endif

#endif // __RCC_NODE_H__
