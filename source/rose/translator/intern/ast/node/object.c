#include "base.h"

#include "RT_context.h"
#include "RT_token.h"
#include "RT_object.h"

typedef struct RCCNodeObject {
	RCCNode base;
	
	const struct RCCObject *object;
} RCCNodeObject;

/* -------------------------------------------------------------------- */
/** \name Create Object Node
 * \{ */

ROSE_INLINE RCCNode *RT_node_new_obj(RCContext *C, int type, const RCCObject *object) {
	RCCNodeObject *node = RT_node_new(C, object->identifier, object->type, NODE_OBJECT, type, sizeof(RCCNodeObject));
	
	node->object = object;
	
	return (RCCNode *)node;
}

RCCNode *RT_node_new_variable(RCContext *C, const RCCObject *object) {
	ROSE_assert(object->kind == OBJ_VARIABLE);
	
	return RT_node_new_obj(C, object->kind, object);
}
RCCNode *RT_node_new_typedef(RCContext *C, const RCCObject *object) {
	ROSE_assert(object->kind == OBJ_TYPEDEF);

	return RT_node_new_obj(C, object->kind, object);
}
RCCNode *RT_node_new_function(RCContext *C, const RCCObject *object) {
	ROSE_assert(object->kind == OBJ_FUNCTION);
	
	return RT_node_new_obj(C, object->kind, object);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Node Utils
 * \{ */

const struct RCCObject *RT_node_obj(const struct RCCNode *node) {
	ROSE_assert(node->kind == NODE_OBJECT);

	return ((const struct RCCNodeObject *)node)->object;
}

/** \} */
