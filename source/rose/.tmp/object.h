#ifndef __RCC_OBJECT_H__
#define __RCC_OBJECT_H__

#include "LIB_utildefines.h"

#include "token.h"
#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/** Note that this is only the tip of the iceberg, this is only the base class! */
typedef struct RCCObject {
	struct RCCObject *prev, *next;
	
	/** The evaluated name of the variable or the function. */
	char *name;
	
	/** The type of the object, this pointer is owned by the object. */
	RCCType *type;
	
	const struct RCCToken *token;
	const struct RCCNode *node;
} RCCObject;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

/** Unsafe function creation method should be avoided for variables! */
RCCObject *RCC_object_new(const char *name, const RCCToken *token);

RCCObject *RCC_object_new_variable(const char *name, RCCType *type);
RCCObject *RCC_object_new_gvariable(const char *name, RCCType *type);
RCCObject *RCC_object_new_string_literal(const char *name, RCCType *type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RCC_object_free(RCCObject *object);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Flag Management
 * \{ */

bool RCC_object_is_function(const RCCObject *object);
bool RCC_object_is_variable(const RCCObject *object);

bool RCC_object_is_definition(const RCCObject *object);
bool RCC_object_is_extern(const RCCObject *object);
bool RCC_object_is_inline(const RCCObject *object);
bool RCC_object_is_live(const RCCObject *object);
bool RCC_object_is_root(const RCCObject *object);
bool RCC_object_is_static(const RCCObject *object);
bool RCC_object_is_tentative(const RCCObject *object);
bool RCC_object_is_threadlocal(const RCCObject *object);
bool RCC_object_is_typedef(const RCCObject *object);

void RCC_object_mark_definition(RCCObject *object, bool value);
void RCC_object_mark_extern(RCCObject *object, bool value);
void RCC_object_mark_inline(RCCObject *object, bool value);
void RCC_object_mark_live(RCCObject *object, bool value);
void RCC_object_mark_root(RCCObject *object, bool value);
void RCC_object_mark_static(RCCObject *object, bool value);
void RCC_object_mark_tentative(RCCObject *object, bool value);
void RCC_object_mark_threadlocal(RCCObject *object, bool value);
void RCC_object_mark_typedef(RCCObject *object, bool value);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Management
 * \{ */

/** This is for variables, returns the data of the variable for string literals ONLY! */
void *RCC_object_data(const RCCObject *object);

size_t RCC_object_get_align(const RCCObject *object);
void RCC_object_set_align(RCCObject *object, size_t align);

/** Local variable memory will be handlded by the root object. */
void RCC_object_add_local(RCCObject *object, RCCObject *local);
/** Local parameter memory will be handlded by the root object. */
void RCC_object_add_param(RCCObject *object, RCCObject *param);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // __RCC_OBJECT_H__
