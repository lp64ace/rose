#include "base.h"
#include "context.h"
#include "object.h"
#include "token.h"

#include "ast/type.h"

typedef struct RCCNodeDeclaration {
	RCCNode base;
	
	const struct RCCType *type;
	const struct RCCObject *object;
} RCCNodeDeclaration;

/* -------------------------------------------------------------------- */
/** \name Create Declaration Node
 * \{ */

RCCNode *RCC_node_new_declaration(RCContext *C, const RCCToken *token, const RCCType *type, const RCCObject *object) {
	RCCNodeDeclaration *node = RCC_node_new(C, token, NODE_DECLARATION, 0, sizeof(RCCNodeDeclaration));
	
	node->type = type;
	node->object = object;
	
	return (RCCNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Declaration Node Utils
 * \{ */



/** \} */
