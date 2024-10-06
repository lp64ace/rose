#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"
#include "LIB_string.h"

#include "object.h"

#define DEFAULT_OBJECT_NAME_LENGTH 64

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCVariable {
	RCCObject object;
	
	unsigned int is_static : 1;
	unsigned int is_definition : 1;
	unsigned int is_extern : 1;
	unsigned int is_tentative : 1;
	unsigned int is_threadlocal : 1;
	unsigned int is_typedef : 1;
	
	/** If this is a static string literal this is the representitive string data. */
	void *data;
	
	size_t align;
} RCCVariable;

typedef struct RCCFunction {
	RCCObject object;
	
	unsigned int is_static : 1;
	unsigned int is_inline : 1;
	unsigned int is_live : 1;
	unsigned int is_root : 1;
	
	size_t stack;
	
	/** The list of parameters (objects) of the function. */
	ListBase params;
	/** The list of local variables (objects) of the function. */
	ListBase locals;
} RCCFunction;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

RCCObject *RCC_object_new(const char *name, const RCCToken *token) {
	const size_t size = ROSE_MAX(sizeof(RCCVariable), sizeof(RCCFunction));
	RCCObject *object = MEM_callocN(size, "RCCObject");
	
	if(name) {
		object->name = LIB_strdupN(name);
	}
	else {
		object->name = MEM_callocN(DEFAULT_OBJECT_NAME_LENGTH, "RCCObject::name");
		
		/** We ensure unique name among all active objects using the pointer to the object. */
		LIB_strformat(object->name, DEFAULT_OBJECT_NAME_LENGTH, "__unamed%p", object);
	}
	
	/**
	 * This type might be invalid when creating the object, but before accessing or managing 
	 * internal data, this has to have been set beforehand!
 	 */
	object->type = NULL;
	
	return object;
}

RCCObject *RCC_object_new_variable(const char *name, RCCType *type) {
	RCCVariable *variable = (RCCVariable * )RCC_object_new(name, NULL);
	
	variable->object.type = type;
	variable->align = RCC_type_align(type);
	
	return (RCCObject *)variable;
}
RCCObject *RCC_object_new_gvariable(const char *name, RCCType *type) {
	RCCVariable *variable = (RCCVariable * )RCC_object_new(name, NULL);
	
	variable->object.type = type;
	variable->align = RCC_type_align(type);
	
	variable->is_definition = true;
	variable->is_static = true;
	
	return (RCCObject *)variable;
}
RCCObject *RCC_object_new_string_literal(const char *p, RCCType *type) {
	RCCVariable *variable = (RCCVariable * )RCC_object_new_gvariable(NULL, type);
	
	variable->data = LIB_strdupN(p);
	
	return (RCCObject *)variable;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

ROSE_INLINE void RCC_object_clear_function(RCCFunction *func) {
	LISTBASE_FOREACH(RCCObject *, p, &func->params) {
		RCC_object_free(p);
	}
	LISTBASE_FOREACH(RCCObject *, l, &func->locals) {
		RCC_object_free(l);
	}
	
	LIB_listbase_clear(&func->params);
	LIB_listbase_clear(&func->locals);
}

ROSE_INLINE void RCC_object_clear_variable(RCCVariable *var) {
	if(var->data) {
		/** The initialization data are owned by the object. */
		MEM_freeN(var->data);
	}
}

void RCC_object_free(RCCObject *object) {
	if(RCC_object_is_function(object)) {
		RCC_object_clear_function(RCC_object_as_function(object));
	}
	else {
		RCC_object_clear_variable(RCC_object_as_varialbe(object));
	}
	
	/** The objects are the owners of types. */
	RCC_type_free(object->type);
	
	if(object->name) {
		/** The name of the object is either a duplicate string or a string owned by the object. */
		MEM_freeN(object->name);
	}
	
	MEM_freeN(object);
}

/** \} */

ROSE_INLINE const RCCVariable *RCC_cobject_as_varialbe(const RCCObject *object) {
	ROSE_assert(RCC_object_is_variable(object));
	
	return (const RCCVariable *)object;
}
ROSE_INLINE const RCCFunction *RCC_cobject_as_function(const RCCObject *object) {
	ROSE_assert(RCC_object_is_function(object));
	
	return (const RCCFunction *)object;
}

ROSE_INLINE RCCVariable *RCC_object_as_varialbe(RCCObject *object) {
	ROSE_assert(RCC_object_is_variable(object));
	
	return (RCCVariable *)object;
}
ROSE_INLINE RCCFunction *RCC_object_as_function(RCCObject *object) {
	ROSE_assert(RCC_object_is_function(object));
	
	return (RCCFunction *)object;
}

/* -------------------------------------------------------------------- */
/** \name Flag Management (Get)
 * \{ */

bool RCC_object_is_function(const RCCObject *object) {
	return RCC_type_kind(object->type, TP_FUNC);
}
bool RCC_object_is_variable(const RCCObject *object) {
	/** Anything else is considered to be a variable, even a pointer to a function! */
	return !RCC_type_kind(object->type, TP_FUNC);
}

bool RCC_object_is_definition(const RCCObject *object) {
	const RCCVariable *var = RCC_cobject_as_varialbe(object);
	
	return var->is_definition;
}
bool RCC_object_is_extern(const RCCObject *object) {
	const RCCVariable *var = RCC_cobject_as_varialbe(object);
	
	return var->is_extern;
}
bool RCC_object_is_inline(const RCCObject *object) {
	const RCCFunction *func = RCC_cobject_as_function(object);
	
	return func->is_inline;
}
bool RCC_object_is_live(const RCCObject *object) {
	const RCCFunction *func = RCC_cobject_as_function(object);
	
	return func->is_live;
}
bool RCC_object_is_root(const RCCObject *object) {
	const RCCFunction *func = RCC_cobject_as_function(object);
	
	return func->is_root;
}
bool RCC_object_is_static(const RCCObject *object) {
	if(RCC_object_is_function(object)) {
		RCCFunction *func = RCC_object_as_function(object);
		
		return func->is_static;
	}
	else {
		RCCVariable *var = RCC_object_as_varialbe(object);
	
		return var->is_static;
	}
}
bool RCC_object_is_tentative(const RCCObject *object) {
	const RCCVariable *var = RCC_cobject_as_varialbe(object);
	
	return var->is_tentative;
}
bool RCC_object_is_threadlocal(const RCCObject *object) {
	RCCVariable *var = RCC_cobject_as_varialbe(object);
	
	return var->is_threadlocal;
}
bool RCC_object_is_typedef(const RCCObject *object) {
	RCCVariable *var = RCC_cobject_as_varialbe(object);
	
	return var->is_typedef;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Flag Management (Set)
 * \{ */

void RCC_object_mark_definition(RCCObject *object, bool value) {
	RCCVariable *var = RCC_object_as_varialbe(object);
		
	var->is_definition = value;
}
void RCC_object_mark_extern(RCCObject *object, bool value) {
	RCCVariable *var = RCC_object_as_varialbe(object);
	
	var->is_extern = value;
}
void RCC_object_mark_inline(RCCObject *object, bool value) {
	RCCFunction *func = RCC_object_as_function(object);
	
	func->is_inline = value;
}
void RCC_object_mark_live(RCCObject *object, bool value) {
	RCCFunction *func = RCC_object_as_function(object);
	
	func->is_live = value;
}
void RCC_object_mark_root(RCCObject *object, bool value) {
	RCCFunction *func = RCC_object_as_function(object);
	
	func->is_root = value;
}
void RCC_object_mark_static(RCCObject *object, bool value) {
	if(RCC_object_is_function(object)) {
		RCCFunction *func = (RCCFunction *)object;
		
		func->is_static = value;
	}
	else {
		RCCVariable *var = (RCCVariable *)object;
		
		var->is_static = value;
	}
}
void RCC_object_mark_tentative(RCCObject *object, bool value) {
	RCCVariable *var = RCC_object_as_varialbe(object);
	
	var->is_tentative = value;
}
void RCC_object_mark_threadlocal(RCCObject *object, bool value) {
	RCCVariable *var = RCC_object_as_varialbe(object);
	
	var->is_threadlocal = value;
}
void RCC_object_mark_typedef(RCCObject *object, bool value) {
	RCCVariable *var = RCC_object_as_varialbe(object);
	
	var->is_typedef = value;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Management
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name Variable Management
 * \{ */

void *RCC_object_data(const RCCObject *object) {
	RCCVariable *var = RCC_cobject_as_varialbe(object);
	
	return var->data;
}

size_t RCC_object_get_align(const RCCObject *object) {
	RCCVariable *var = RCC_cobject_as_varialbe(object);
	
	return var->align;
}
void RCC_object_set_align(RCCObject *object, size_t align) {
	RCCVariable *var = RCC_cobject_as_varialbe(object);
	
	var->align = align;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Function Management
 * \{ */

void RCC_object_add_local(RCCObject *object, RCCObject *local) {
	RCCFunction *func = RCC_cobject_as_function(object);
	
	LIB_addtail(&func->locals, local);
}
void RCC_object_add_param(RCCObject *object, RCCObject *param) {
	RCCFunction *func = RCC_cobject_as_function(object);
	
	LIB_addtail(&func->params, param);
}

/** \} */
