#include "context.h"
#include "object.h"
#include "token.h"

#include "ast/node.h"
#include "ast/type.h"

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

ROSE_INLINE RCCObject *object_new(RCContext *C, int kind, const RCCType *type, const RCCToken *token) {
	RCCObject *object = RCC_context_calloc(C, sizeof(RCCObject));

	object->kind = kind;
	object->type = type;
	object->identifier = token;
	object->body = NULL;

	return object;
}

RCCObject *RCC_object_new_variable(RCContext *C, const RCCType *type, const RCCToken *token) {
	RCCObject *object = object_new(C, OBJ_VARIABLE, type, token);
	return object;
}
RCCObject *RCC_object_new_typedef(RCContext *C, const RCCType *type, const RCCToken *token) {
	RCCObject *object = object_new(C, OBJ_TYPEDEF, type, token);
	return object;
}
RCCObject *RCC_object_new_function(RCContext *C, const RCCType *type, const RCCToken *token, const RCCNode *node) {
	RCCObject *object = object_new(C, OBJ_FUNCTION, type, token);
	object->body = node;
	return object;
}

/** \} */