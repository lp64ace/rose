#include "RT_context.h"
#include "RT_object.h"
#include "RT_token.h"

#include "ast/node.h"
#include "ast/type.h"

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

ROSE_INLINE RTObject *object_new(RTContext *C, int kind, const RTType *type, const RTToken *token) {
	RTObject *object = RT_context_calloc(C, sizeof(RTObject));

	object->kind = kind;
	object->type = type;
	object->identifier = token;
	object->body = NULL;

	return object;
}

RTObject *RT_object_new_variable(RTContext *C, const RTType *type, const RTToken *token) {
	RTObject *object = object_new(C, OBJ_VARIABLE, type, token);
	return object;
}
RTObject *RT_object_new_typedef(RTContext *C, const RTType *type, const RTToken *token) {
	RTObject *object = object_new(C, OBJ_TYPEDEF, type, token);
	return object;
}
RTObject *RT_object_new_function(RTContext *C, const RTType *type, const RTToken *token, const RTNode *node) {
	RTObject *object = object_new(C, OBJ_FUNCTION, type, token);
	object->body = node;
	return object;
}
RTObject *RT_object_new_enum(RTContext *C, const RTType *type, const RTToken *token, const RTNode *value) {
	RTObject *object = object_new(C, OBJ_ENUM, type, token);
	object->body = value;
	return object;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_object_is_variable(const RTObject *object) {
	return object->kind == OBJ_VARIABLE;
}
bool RT_object_is_typedef(const RTObject *object) {
	return object->kind == OBJ_TYPEDEF;
}
bool RT_object_is_function(const RTObject *object) {
	return object->kind == OBJ_FUNCTION;
}
bool RT_object_is_enum(const RTObject *object) {
	return object->kind == OBJ_ENUM;
}

/** \} */
