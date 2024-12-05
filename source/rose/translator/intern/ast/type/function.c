#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

ROSE_INLINE bool same_function_type(const RTType *a, const RTType *b) {
	if (!(a->kind == TP_FUNC && b->kind == TP_FUNC)) {
		return false;
	}
	const RTTypeFunction *f1 = &a->tp_function, *f2 = &b->tp_function;
	if (!RT_type_same(f1->return_type, f2->return_type)) {
		return false;
	}
	if (!(f1->is_complete == f2->is_complete)) {
		return false;
	}
	if (f1->is_complete) {
		const RTTypeParameter *iter1 = (const RTTypeParameter *)f1->parameters.first;
		const RTTypeParameter *iter2 = (const RTTypeParameter *)f2->parameters.first;
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

ROSE_INLINE bool compatible_function_type(const RTType *a, const RTType *b) {
	if (!(a->kind == TP_FUNC && b->kind == TP_FUNC)) {
		return false;
	}
	const RTTypeFunction *f1 = &a->tp_function, *f2 = &b->tp_function;
	if (!RT_type_compatible(f1->return_type, f2->return_type)) {
		return false;
	}
	if (!(f1->is_complete == f2->is_complete)) {
		return false;
	}
	if (f1->is_complete) {
		const RTTypeParameter *iter1 = (const RTTypeParameter *)f1->parameters.first;
		const RTTypeParameter *iter2 = (const RTTypeParameter *)f2->parameters.first;
		while (iter1 && iter2) {
			const RTType *unqualified1 = RT_type_unqualified(iter1->adjusted);
			const RTType *unqualified2 = RT_type_unqualified(iter2->adjusted);
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

ROSE_INLINE const RTType *composite_function_type(RTContext *C, const RTType *a, const RTType *b) {
	if (!(a->kind == TP_FUNC && b->kind == TP_FUNC)) {
		return NULL;
	}
	if (!RT_type_compatible(a, b)) {
		return NULL;
	}
	const RTTypeFunction *f1 = &a->tp_function, *f2 = &b->tp_function;

	struct RTType *composite = RT_type_new_function(C, RT_type_composite(C, f1->return_type, f2->return_type));
	const RTTypeParameter *iter1 = (const RTTypeParameter *)f1->parameters.first;
	const RTTypeParameter *iter2 = (const RTTypeParameter *)f2->parameters.first;
	while (iter1 && iter2) {
		if (!RT_token_match(iter1->identifier, iter2->identifier)) {
			return false;
		}
		const RTType *unqualified1 = RT_type_unqualified(iter1->adjusted);
		const RTType *unqualified2 = RT_type_unqualified(iter2->adjusted);
		/** Try to create a parameter from the unqualified adjusted parameter types. */
		const RTType *type = RT_type_composite(C, unqualified1, unqualified2);
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

RTType *RT_type_new_function(RTContext *C, const RTType *type) {
	RTType *f = RT_context_calloc(C, sizeof(RTType));

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

ROSE_INLINE const RTType *adjust_function_parameter(RTContext *C, const RTType *type) {
	const RTType *unqualified = RT_type_unqualified(type);
	const RTType *adjusted = NULL;

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

void RT_type_function_add_named_parameter(RTContext *C, RTType *t, const RTType *type, const RTToken *identifier) {
	ROSE_assert(t->kind == TP_FUNC);
	RTTypeFunction *f = &t->tp_function;
	RTTypeParameter *param = RT_context_calloc(C, sizeof(RTTypeParameter));

	param->identifier = identifier;
	param->type = type;
	param->adjusted = adjust_function_parameter(C, type);

	const RTTypeParameter *last = (const RTTypeParameter *)f->parameters.last;
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
void RT_type_function_add_parameter(RTContext *C, RTType *f, const RTType *type) {
	RT_type_function_add_named_parameter(C, f, type, NULL);
}
void RT_type_function_add_ellipsis_parameter(RTContext *C, RTType *f) {
	RT_type_function_add_parameter(C, f, Tp_Ellipsis);
}

void RT_type_function_finalize(RTContext *C, RTType *f) {
	ROSE_assert(f->kind == TP_FUNC);

	if (LIB_listbase_is_empty(&f->tp_function.parameters)) {
		f->tp_function.is_variadic = true;
	}
	f->tp_function.is_complete = true;
}

/** \} */
