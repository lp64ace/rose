#include "ast/type.h"

#include "RT_context.h"

ROSE_INLINE bool same_qualified_type(const RTType *a, const RTType *b) {
	if (!(a->kind == TP_QUALIFIED && b->kind == TP_QUALIFIED)) {
		return false;
	}
	const RTTypeQualified *q1 = &a->tp_qualified, *q2 = &b->tp_qualified;
	if (!RT_type_same(q1->base, q2->base)) {
		return false;
	}
	const RTTypeQualification *qualification1 = RT_type_qualification(a);
	const RTTypeQualification *qualification2 = RT_type_qualification(b);
	if (!(qualification1->is_constant == qualification2->is_constant)) {
		return false;
	}
	if (!(qualification1->is_restricted == qualification2->is_restricted)) {
		return false;
	}
	if (!(qualification1->is_volatile == qualification2->is_volatile)) {
		return false;
	}
	if (!(qualification1->is_atomic == qualification2->is_atomic)) {
		return false;
	}
	/**
	 * We compare everything one by one because this is a bitfield,
	 * hence we are not aware if the rest of the bytes are initialized.
	 * Since every type is allocated with calloc and is not reused,
	 * the remaining bytes should also be zero so `memcmp` should be sufficient.
	 * Leave this check only for debug for now, just to see if any extra fields are added.
	 */
	ROSE_assert(memcmp(qualification1, qualification2, sizeof(RTTypeQualification)) == 0);
	return true;
}

ROSE_INLINE bool compatible_qualified_type(const RTType *a, const RTType *b) {
	if (a->kind == TP_QUALIFIED && b->kind != TP_QUALIFIED) {
		return RT_type_is_zero_qualified(a) && RT_type_compatible(RT_type_unqualified(a), b);
	}
	if (!(a->kind == TP_QUALIFIED && b->kind == TP_QUALIFIED)) {
		return false;
	}
	const RTTypeQualified *q1 = &a->tp_qualified, *q2 = &b->tp_qualified;
	if (!RT_type_compatible(q1->base, q2->base)) {
		return false;
	}
	const RTTypeQualification *qualification1 = RT_type_qualification(a);
	const RTTypeQualification *qualification2 = RT_type_qualification(b);
	if (!(qualification1->is_constant == qualification2->is_constant)) {
		return false;
	}
	if (!(qualification1->is_restricted == qualification2->is_restricted)) {
		return false;
	}
	if (!(qualification1->is_volatile == qualification2->is_volatile)) {
		return false;
	}
	if (!(qualification1->is_atomic == qualification2->is_atomic)) {
		return false;
	}
	/**
	 * We compare everything one by one because this is a bitfield,
	 * hence we are not aware if the rest of the bytes are initialized.
	 * Since every type is allocated with calloc and is not reused,
	 * the remaining bytes should also be zero so `memcmp` should be sufficient.
	 * Leave this check only for debug for now, just to see if any extra fields are added.
	 */
	ROSE_assert(memcmp(qualification1, qualification2, sizeof(RTTypeQualification)) == 0);
	return true;
}

ROSE_INLINE const struct RTType *composite_qualified_type(struct RTContext *C, const struct RTType *a, const struct RTType *b) {
	if (!RT_type_compatible(a, b)) {
		return NULL;
	}
	const RTTypeQualification *qualification = &a->tp_qualified.qualification;
	a = RT_type_unqualified(a);
	b = RT_type_unqualified(b);
	struct RTType *composite = RT_type_new_qualified(C, RT_type_composite(C, a, b));
	/** Copy the left qualification over to the new type. */
	memcpy(&composite->tp_qualified.qualification, qualification, sizeof(RTTypeQualification));
	return composite;
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

ROSE_INLINE RTType *qualified_new(struct RTContext *C, const struct RTType *type) {
	RTType *q = RT_context_calloc(C, sizeof(RTType));

	q->kind = TP_QUALIFIED;
	q->tp_qualified.base = type;

	q->same = same_qualified_type;
	q->compatible = compatible_qualified_type;
	q->composite = composite_qualified_type;

	return q;
}

RTType *RT_type_new_qualified(RTContext *C, const RTType *type) {
	RTType *q = qualified_new(C, type);

	return q;
}

RTType *RT_type_new_qualified_ex(RTContext *C, const RTType *type, const RTTypeQualification *src) {
	RTType *q = qualified_new(C, type);

	RTTypeQualification *dst = &q->tp_qualified.qualification;

	/**
	 * Make this a memcpy and the program will break (in Debug mode, at least)!
	 * We need the rest of the bitfield to remain zero initialized,
	 * so if `src` was not zero allocated or zero initialized there will be problems!
	 */
	dst->is_constant = src->is_constant;
	dst->is_restricted = src->is_restricted;
	dst->is_volatile = src->is_volatile;
	dst->is_atomic = src->is_atomic;

	return q;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RT_type_qualified_const(struct RTType *qual) {
	ROSE_assert(qual->kind == TP_QUALIFIED);
	if (qual->kind == TP_QUALIFIED) {
		qual->tp_qualified.qualification.is_constant = true;
	}
}
void RT_type_qualified_restrict(struct RTType *qual) {
	ROSE_assert(qual->kind == TP_QUALIFIED);
	if (qual->kind == TP_QUALIFIED) {
		qual->tp_qualified.qualification.is_restricted = true;
	}
}
void RT_type_qualified_volatile(struct RTType *qual) {
	ROSE_assert(qual->kind == TP_QUALIFIED);
	if (qual->kind == TP_QUALIFIED) {
		qual->tp_qualified.qualification.is_volatile = true;
	}
}
void RT_type_qualified_atomic(struct RTType *qual) {
	ROSE_assert(qual->kind == TP_QUALIFIED);
	if (qual->kind == TP_QUALIFIED) {
		qual->tp_qualified.qualification.is_atomic = true;
	}
}

bool RT_type_qualified_is_const(const struct RTType *qual) {
	if (qual->kind == TP_QUALIFIED) {
		return qual->tp_qualified.qualification.is_constant;
	}
	return false;
}
bool RT_type_qualified_is_restrict(const struct RTType *qual) {
	if (qual->kind == TP_QUALIFIED) {
		return qual->tp_qualified.qualification.is_restricted;
	}
	return false;
}
bool RT_type_qualified_is_volatile(const struct RTType *qual) {
	if (qual->kind == TP_QUALIFIED) {
		return qual->tp_qualified.qualification.is_volatile;
	}
	return false;
}
bool RT_type_qualified_is_atomic(const struct RTType *qual) {
	if (qual->kind == TP_QUALIFIED) {
		return qual->tp_qualified.qualification.is_atomic;
	}
	return false;
}

const struct RTType *RT_type_unqualified(const struct RTType *type) {
	if (type->kind == TP_QUALIFIED) {
		return type->tp_qualified.base;
	}
	return type;
}
const struct RTTypeQualification *RT_type_qualification(const struct RTType *type) {
	static RTTypeQualification zero_qualification = {
		.is_constant = false,
		.is_restricted = false,
		.is_volatile = false,
		.is_atomic = false,
	};
	if (type && type->kind == TP_QUALIFIED) {
		return &type->tp_qualified.qualification;
	}
	return &zero_qualification;
}

bool RT_type_is_zero_qualified(const struct RTType *type) {
	if (type->kind == TP_QUALIFIED) {
		const RTTypeQualification *qual = &type->tp_qualified.qualification;

		if (!RT_qual_is_empty(qual)) {
			return false;
		}
	}
	return true;
}

/** \} */
