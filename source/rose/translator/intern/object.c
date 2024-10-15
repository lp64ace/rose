#include "RT_context.h"
#include "RT_object.h"
#include "RT_token.h"

#include "ast/node.h"
#include "ast/type.h"

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

ROSE_INLINE RCCObject *object_new(RCContext *C, int kind, const RCCType *type, const RCCToken *token) {
	RCCObject *object = RT_context_calloc(C, sizeof(RCCObject));

	object->kind = kind;
	object->type = type;
	object->identifier = token;
	object->body = NULL;

	return object;
}

RCCObject *RT_object_new_variable(RCContext *C, const RCCType *type, const RCCToken *token) {
	RCCObject *object = object_new(C, OBJ_VARIABLE, type, token);
	return object;
}
RCCObject *RT_object_new_typedef(RCContext *C, const RCCType *type, const RCCToken *token) {
	RCCObject *object = object_new(C, OBJ_TYPEDEF, type, token);
	return object;
}
RCCObject *RT_object_new_function(RCContext *C, const RCCType *type, const RCCToken *token, const RCCNode *node) {
	RCCObject *object = object_new(C, OBJ_FUNCTION, type, token);
	object->body = node;
	return object;
}
RCCObject *RT_object_new_enum(RCContext *C, const RCCType *type, const RCCToken *token, const RCCNode *value) {
	RCCObject *object = object_new(C, OBJ_ENUM, type, token);
	object->body = value;
	return object;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_object_is_variable(const RCCObject *object) {
	return object->kind == OBJ_VARIABLE;
}
bool RT_object_is_typedef(const RCCObject *object) {
	return object->kind == OBJ_TYPEDEF;
}
bool RT_object_is_function(const RCCObject *object) {
	return object->kind == OBJ_FUNCTION;
}
bool RT_object_is_enum(const RCCObject *object) {
	return object->kind == OBJ_ENUM;
}

/** \} */
