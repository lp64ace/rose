#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

ROSE_INLINE bool same_function_type(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_FUNC && b->kind == TP_FUNC)) {
		return false;
	}
	const RCCTypeFunction *f1 = &a->tp_function, *f2 = &b->tp_function;
	if (!RT_type_same(f1->return_type, f2->return_type)) {
		return false;
	}
	if (!(f1->is_complete == f2->is_complete)) {
		return false;
	}
	if (f1->is_complete) {
		const RCCTypeParameter *iter1 = (const RCCTypeParameter *)f1->parameters.first;
		const RCCTypeParameter *iter2 = (const RCCTypeParameter *)f2->parameters.first;
		while (iter1 && iter2) {
			if (!RT_token_match(iter1->identifier, iter2->identifier)) {
				return false;
			}
			if (!RT_type_same(iter1->type, iter2->type)) {
				return false;
			}
			iter1 = iter1->next;
			iter2 = iter2->next;
		}
		return iter1 == NULL && iter2 == NULL;
	}
	return true;
}

ROSE_INLINE bool compatible_function_type(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_FUNC && b->kind == TP_FUNC)) {
		return false;
	}
	const RCCTypeFunction *f1 = &a->tp_function, *f2 = &b->tp_function;
	if (!RT_type_compatible(f1->return_type, f2->return_type)) {
		return false;
	}
	if (!(f1->is_complete == f2->is_complete)) {
		return false;
	}
	if (f1->is_complete) {
		const RCCTypeParameter *iter1 = (const RCCTypeParameter *)f1->parameters.first;
		const RCCTypeParameter *iter2 = (const RCCTypeParameter *)f2->parameters.first;
		while (iter1 && iter2) {
			const RCCType *unqualified1 = RT_type_unqualified(iter1->adjusted);
			const RCCType *unqualified2 = RT_type_unqualified(iter2->adjusted);
			if (!RT_type_compatible(unqualified1, unqualified2)) {
				return false;
			}
			iter1 = iter1->next;
			iter2 = iter2->next;
		}
		return iter1 == NULL && iter2 == NULL;
	}
	return true;
}

ROSE_INLINE const RCCType *composite_function_type(RCContext *C, const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_FUNC && b->kind == TP_FUNC)) {
		return NULL;
	}
	if (!RT_type_compatible(a, b)) {
		return NULL;
	}
	const RCCTypeFunction *f1 = &a->tp_function, *f2 = &b->tp_function;

	struct RCCType *composite = RT_type_new_function(C, RT_type_composite(C, f1->return_type, f2->return_type));
	const RCCTypeParameter *iter1 = (const RCCTypeParameter *)f1->parameters.first;
	const RCCTypeParameter *iter2 = (const RCCTypeParameter *)f2->parameters.first;
	while (iter1 && iter2) {
		if (!RT_token_match(iter1->identifier, iter2->identifier)) {
			return false;
		}
		const RCCType *unqualified1 = RT_type_unqualified(iter1->adjusted);
		const RCCType *unqualified2 = RT_type_unqualified(iter2->adjusted);
		/** Try to create a parameter from the unqualified adjusted parameter types. */
		const RCCType *type = RT_type_composite(C, unqualified1, unqualified2);
		ROSE_assert(type != NULL);

		if (!RT_token_match(iter1->identifier, iter2->identifier)) {
			/** If the names do not much just add unnamed parameter. */
			RT_type_function_add_parameter(C, composite, type);
		}
		else {
			RT_type_function_add_named_parameter(C, composite, type, iter1->identifier);
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

RCCType *RT_type_new_function(RCContext *C, const RCCType *type) {
	RCCType *f = RT_context_calloc(C, sizeof(RCCType));

	f->kind = TP_FUNC;
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
	const RCCType *unqualified = RT_type_unqualified(type);
	const RCCType *adjusted = NULL;

	switch (unqualified->kind) {
		case TP_FUNC: {
			/** Functions are treated like pointers to a function. */
			adjusted = RT_type_new_pointer(C, unqualified);
		} break;
		case TP_ARRAY: {
			/** Arrays are treated like a pointer to the first element. */
			adjusted = RT_type_new_pointer(C, unqualified->tp_array.element_type);
			if (!RT_qual_is_empty(&unqualified->tp_array.qualification)) {
				adjusted = RT_type_new_qualified_ex(C, adjusted, &unqualified->tp_array.qualification);
			}
		} break;
		case TP_VARIADIC: {
			/** Variadic types are treated like pointers! */
			adjusted = RT_type_new_pointer(C, unqualified);
		} break;
		default: {
			adjusted = type;
		} break;
	}

	if (type->kind == TP_QUALIFIED && adjusted->kind != TP_QUALIFIED) {
		/** Copy over the qualifications back to the type. */
		adjusted = RT_type_new_qualified_ex(C, adjusted, &type->tp_qualified.qualification);
	}

	return adjusted;
}

void RT_type_function_add_named_parameter(RCContext *C, RCCType *t, const RCCType *type, const RCCToken *identifier) {
	ROSE_assert(t->kind == TP_FUNC);
	RCCTypeFunction *f = &t->tp_function;
	RCCTypeParameter *param = RT_context_calloc(C, sizeof(RCCTypeParameter));

	param->identifier = identifier;
	param->type = type;
	param->adjusted = adjust_function_parameter(C, type);

	const RCCTypeParameter *last = (const RCCTypeParameter *)f->parameters.last;
	if (last && RT_type_same(last->type, Tp_Ellipsis)) {
		/**
		 * Only one ellispis is allowed and it has to be the last argument of a function!
		 * This is an assert and not a compiler error because we do not have a token as to where to post the error,
		 * since `identifier` may be NULL.
		 */
		ROSE_assert_unreachable();
	}
	LIB_addtail(&f->parameters, param);
}
void RT_type_function_add_parameter(RCContext *C, RCCType *f, const RCCType *type) {
	RT_type_function_add_named_parameter(C, f, type, NULL);
}
void RT_type_function_add_ellipsis_parameter(RCContext *C, RCCType *f) {
	RT_type_function_add_parameter(C, f, Tp_Ellipsis);
}

void RT_type_function_finalize(RCContext *C, RCCType *f) {
	ROSE_assert(f->kind == TP_FUNC);

	if (LIB_listbase_is_empty(&f->tp_function.parameters)) {
		f->tp_function.is_variadic = true;
	}
	f->tp_function.is_complete = true;
}

/** \} */
