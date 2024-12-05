#include "base.h"

#include "RT_context.h"
#include "RT_object.h"
#include "RT_token.h"

typedef struct RTNodeObject {
	RTNode base;

	const struct RTObject *object;
} RTNodeObject;

/* -------------------------------------------------------------------- */
/** \name Create Object Node
 * \{ */

ROSE_INLINE RTNode *RT_node_new_obj(RTContext *C, int type, const RTObject *object) {
	RTNodeObject *node = RT_node_new(C, object->identifier, object->type, NODE_OBJECT, type, sizeof(RTNodeObject));

	node->object = object;

	return (RTNode *)node;
}

RTNode *RT_node_new_variable(RTContext *C, const RTObject *object) {
	ROSE_assert(object->kind == OBJ_VARIABLE);

	return RT_node_new_obj(C, object->kind, object);
}
RTNode *RT_node_new_typedef(RTContext *C, const RTObject *object) {
	ROSE_assert(object->kind == OBJ_TYPEDEF);

	return RT_node_new_obj(C, object->kind, object);
}
RTNode *RT_node_new_function(RTContext *C, const RTObject *object) {
	ROSE_assert(object->kind == OBJ_FUNCTION);

	return RT_node_new_obj(C, object->kind, object);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Node Utils
 * \{ */

const struct RTObject *RT_node_object(const struct RTNode *node) {
	ROSE_assert(node->kind == NODE_OBJECT);

	return ((const struct RTNodeObject *)node)->object;
}

/** \} */
