#include "ast.h."

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name Node Info
 * \{ */

typedef void (*RCCNodeInitData)(RCCNode *node);
typedef void (*RCCNodeFreeData)(RCCNode *node);

typedef struct RCCNodeInfo {
	size_t size;
	
	/**
	 * This can be NULL indicating that a simple memset to zero is enough to init the structure.
	 */
	RCCNodeInitData init_data;
	/**
	 * This can be NULL indicating that there is nothing to do.
	 */
	RCCNodeFreeData free_data;
} RCCNodeInfo;

#define DEFAULT_NODE \
	{ \
		.size = sizeof(RCCNode), \
		.init_data = NULL, \
		.free_data = NULL, \
	}
#define ADVANCED_NODE(sz, init, free) \
	{ \
		.size = sz, \
		.init_data = init, \
		.free_data = free, \
	}

static RCCNodeInfo infos[];

ROSE_INLINE RCCNodeInfo *RCC_node_info(int kind) {
	RCCNodeInfo *info = &infos[kind];
	
	return info;
}

/** \} */

ROSE_INLINE RCCNode *RCC_node_new(int kind, RCCToken *token, RCCType *type) {
	RCCNodeInfo *info = RCC_node_info(kind);
	
	RCCNode *node = MEM_callocN(info->size, "RCCNode");
	
	node->kind = kind;
	node->type = type;
	node->token = token;
	
	if(info->init_data) {
		info->init_data(node);
	}
	
	return node;
}

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

RCCNode *RCC_node_new_cast(RCCNode *expr, RCCType *type) {
	RCCNode *node = RCC_node_new(ND_CAST, NULL, type);
	
	if(node) {
		node->lhs = expr;
	}
	
	return node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RCC_node_free(RCCNode *node) {
	if(node) {
		RCCNodeInfo *info = RCC_node_info(node->kind);
		
		if(node->free_data) {
			info->free_data(node);
		}
		
		RCC_type_free(node->type);
		MEM_freeN(node);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Node Info Declarations
 * \{ */

static RCCNodeInfo infos[] = {
	[ND_NONE] = CUSTOM_NODE(-1, NULL, NULL),
	
	[ND_ADD] = DEFAULT_NODE,
	[ND_SUB] = DEFAULT_NODE,
	[ND_MUL] = DEFAULT_NODE,
	[ND_DIV] = DEFAULT_NODE,
	[ND_MOD] = DEFAULT_NODE,
	[ND_BITAND] = DEFAULT_NODE,
	[ND_BITOR] = DEFAULT_NODE,
	[ND_BITXOR] = DEFAULT_NODE,
	[ND_LSHIFT] = DEFAULT_NODE,
	[ND_RSHIFT] = DEFAULT_NODE,
	[ND_EQ] = DEFAULT_NODE,
	[ND_NE] = DEFAULT_NODE,
	[ND_LT] = DEFAULT_NODE,
	[ND_LE] = DEFAULT_NODE,
	[ND_ASSIGN] = DEFAULT_NODE,
	
	[ND_COND] = ADVANCED_NODE(),
	[ND_COMMA] = ADVANCED_NODE(),
	[ND_MEMBER] = ADVANCED_NODE(),
	[ND_ADDR] = ADVANCED_NODE(),
	[ND_DEREF] = ADVANCED_NODE(),
	
	[ND_NOT] = DEFAULT_NODE,
	[ND_BITNOT] = DEFAULT_NODE,
	[ND_LOGAND] = DEFAULT_NODE,
	[ND_LOGOR] = DEFAULT_NODE,
	[ND_RETURN] = DEFAULT_NODE,
	
	[ND_IF] = ADVANCED_NODE(),
	[ND_FOR] = ADVANCED_NODE(),
	[ND_DO] = ADVANCED_NODE(),
	[ND_SWITCH] = ADVANCED_NODE(),
	[ND_CASE] = ADVANCED_NODE(),
	[ND_BLOCK] = ADVANCED_NODE(),
	[ND_GOTO] = ADVANCED_NODE(),
	[ND_GOTO_EXPR] = ADVANCED_NODE(),
	[ND_LABEL] = ADVANCED_NODE(),
	[ND_LABEL_VAL] = ADVANCED_NODE(),
	[ND_FUNCALL] = ADVANCED_NODE(),
	[ND_EXPR_STMT] = ADVANCED_NODE(),
	[ND_STMT_EXPR] = ADVANCED_NODE(),
	[ND_VAR] = ADVANCED_NODE(),
	[ND_NUM] = ADVANCED_NODE(),
	
	[ND_CAST] = DEFAULT_NODE,
	
	[ND_MEMZERO] = ADVANCED_NODE(),
	[ND_ASM] = ADVANCED_NODE(),
	[ND_CAS] = ADVANCED_NODE(),
	[ND_EXCH] = ADVANCED_NODE(),
};

/** \} */
