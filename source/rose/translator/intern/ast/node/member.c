#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RCCNodeMember {
	RCCNode base;

	const RCCNode *owner;
	const RCCField *field;
} RCCNodeMember;

/* -------------------------------------------------------------------- */
/** \name Member Nodes
 * \{ */

RCCNode *RT_node_new_member(RCContext *C, const RCCNode *owner, const RCCField *field) {
	RCCNodeMember *node = RT_node_new(C, owner->token, field->type, NODE_MEMBER, 0, sizeof(RCCNodeMember));

	node->owner = owner;
	node->field = field;

	return (RCCNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Member Node Utils
 * \{ */

bool RT_node_is_bitfield(const RCCNode *node) {
	ROSE_assert(node->kind == NODE_MEMBER);

	return ((const RCCNodeMember *)node)->field->properties.is_bitfield;
}

/** \} */
