#include "LIB_listbase.h"

#include "base.h"

#include "RT_context.h"
#include "RT_token.h"

typedef struct RTNodeMember {
	RTNode base;

	const RTNode *owner;
	const RTField *field;
} RTNodeMember;

/* -------------------------------------------------------------------- */
/** \name Member Nodes
 * \{ */

RTNode *RT_node_new_member(RTContext *C, const RTNode *owner, const RTField *field) {
	RTNodeMember *node = RT_node_new(C, owner->token, field->type, NODE_MEMBER, 0, sizeof(RTNodeMember));

	node->owner = owner;
	node->field = field;

	return (RTNode *)node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Member Node Utils
 * \{ */

bool RT_node_is_bitfield(const RTNode *node) {
	ROSE_assert(node->kind == NODE_MEMBER);

	return ((const RTNodeMember *)node)->field->properties.is_bitfield;
}

/** \} */
