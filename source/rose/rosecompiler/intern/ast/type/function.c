#include "ast/type.h"

#include "context.h"
#include "token.h"

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCTypeFunctionParameter {
	struct RCCTypeFunctionParameter *prev, *next;

	const struct RCCToken *name;
	const struct RCCType *type;
	const struct RCCType *adjusted;
} RCCTypeFunctionParameter;

/** \} */

ROSE_INLINE bool same_function_type(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_FUNC && b->kind == TP_FUNC)) {
		return false;
	}
	const RCCTypeFunction *f1 = &a->tp_function, *f2 = &b->tp_function;
	if (!RCC_type_same(f1->return_type, f2->return_type)) {
		return false;
	}
	const RCCTypeFunctionParameter *iter1 = (const RCCTypeFunctionParameter *)f1->parameters.first;
	const RCCTypeFunctionParameter *iter2 = (const RCCTypeFunctionParameter *)f2->parameters.first;
	while (iter1 && iter2) {
		if (!RCC_token_match(iter1->name, iter2->name)) {
			return false;
		}
		if (!RCC_type_match(iter1->type, iter2->type)) {
			return false;
		}
		iter1 = iter1->next;
		iter2 = iter2->next;
	}
	return iter1 == NULL && iter2 == NULL;
}

ROSE_INLINE bool compatible_function_type(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_FUNC && b->kind == TP_FUNC)) {
		return false;
	}
	const RCCTypeFunction *f1 = &a->tp_function, *f2 = &b->tp_function;
	if (!RCC_type_compatible(f1->return_type, f2->return_type)) {
		return false;
	}
	const RCCTypeFunctionParameter *iter1 = (const RCCTypeFunctionParameter *)f1->parameters.first;
	const RCCTypeFunctionParameter *iter2 = (const RCCTypeFunctionParameter *)f2->parameters.first;
	while (iter1 && iter2) {
		const RCCType *unqualified1 = RCC_type_unqualified(iter1->adjusted);
		const RCCType *unqualified2 = RCC_type_unqualified(iter2->adjusted);
		if (!RCC_type_compatible(unqualified1, unqualified2)) {
			return false;
		}
		iter1 = iter1->next;
		iter2 = iter2->next;
	}
	return iter1 == NULL && iter2 == NULL;
}

ROSE_INLINE const RCCType *composite_function_type(RCContext *C, const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_FUNC && b->kind == TP_FUNC)) {
		return NULL;
	}
	if (!RCC_type_compatible(a, b)) {
		return NULL;
	}
	const RCCTypeFunction *f1 = &a->tp_function, *f2 = &b->tp_function;

	struct RCCType *composite = RCC_type_new_function(C, RCC_type_composite(C, f1->return_type, f2->return_type));
	const RCCTypeFunctionParameter *iter1 = (const RCCTypeFunctionParameter *)f1->parameters.first;
	const RCCTypeFunctionParameter *iter2 = (const RCCTypeFunctionParameter *)f2->parameters.first;
	while (iter1 && iter2) {
		if (!RCC_token_match(iter1->name, iter2->name)) {
			return false;
		}
		const RCCType *unqualified1 = RCC_type_unqualified(iter1->adjusted);
		const RCCType *unqualified2 = RCC_type_unqualified(iter2->adjusted);
		/** Try to create a parameter from the unqualified adjusted parameter types. */
		const RCCType *type = RCC_type_composite(C, unqualified1, unqualified2);
		ROSE_assert(type != NULL);

		if (!RCC_token_match(iter1->name, iter2->name)) {
			/** If the names do not much just add unnamed parameter. */
			RCC_type_function_add_parameter(C, composite, type);
		}
		else {
			RCC_type_function_add_named_parameter(C, composite, type, iter1->name);
		}
		iter1 = iter1->next;
		iter2 = iter2->next;
	}
	ROSE_assert(iter1 == NULL && iter2 == NULL);
	return composite;
}

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

RCCType *RCC_type_new_function(RCContext *C, const RCCType *type) {
	RCCType *f = RCC_context_calloc(C, sizeof(RCCType));

	f->tp_function.return_type = type;

	f->same = same_function_type;
	f->compatible = compatible_function_type;
	f->composite = composite_function_type;

	return f;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

ROSE_INLINE const RCCType *adjust_function_parameter(RCContext *C, const RCCType *type) {
	const RCCType *unqualified = RCC_type_unqualified(type);
	const RCCType *adjusted = type;

	switch (unqualified->kind) {
		case TP_FUNC: {
			/** Functions are treated like pointers to a function. */
			adjusted = RCC_type_new_pointer(C, unqualified);
		} break;
		case TP_ARRAY: {
			/** Arrays are treated like a pointer to the first element. */
			adjusted = RCC_type_new_pointer(C, unqualified->tp_array.element_type);
			if (!RCC_qual_is_empty(&unqualified->tp_array.qualification)) {
				adjusted = RCC_type_new_qualified_ex(C, adjusted, &unqualified->tp_array.qualification);
			}
		} break;
		case TP_VARIADIC: {
			/** Variadic types are treated like pointers! */
			adjusted = RCC_type_new_pointer(C, unqualified);
		} break;
	}

	if (type->kind == TP_QUALIFIED && adjusted->kind != TP_QUALIFIED) {
		/** Copy over the qualifications back to the type. */
		adjusted = RCC_type_new_qualified_ex(C, adjusted, &type->tp_qualified.qualification);
	}

	return adjusted;
}

void RCC_type_function_add_named_parameter(RCContext *C, RCCType *t, const RCCType *type, const RCCToken *name) {
	ROSE_assert(t->kind == TP_FUNC);
	RCCTypeFunction *f = &t->tp_function;
	RCCTypeFunctionParameter *param = RCC_context_calloc(C, sizeof(RCCTypeFunctionParameter));

	param->name = name;
	param->type = type;
	param->adjusted = adjust_function_parameter(C, type);

	const RCCTypeFunctionParameter *last = (const RCCTypeFunctionParameter *)f->parameters.last;
	if (last && RCC_type_same(last->type, Tp_Ellipsis)) {
		/**
		 * Only one ellispis is allowed and it has to be the last argument of a function!
		 * This is an assert and not a compiler error because we do not have a token as to where to post the error,
		 * since `name` may be NULL.
		 */
		ROSE_assert_unreachable();
	}
	LIB_addtail(&f->parameters, param);
}
void RCC_type_function_add_parameter(RCContext *C, RCCType *f, const RCCType *type) {
	RCC_type_function_add_named_parameter(C, f, type, NULL);
}
void RCC_type_function_add_ellipsis_parameter(RCContext *C, RCCType *f) {
	RCC_type_function_add_parameter(C, f, Tp_Ellipsis);
}

/** \} */
