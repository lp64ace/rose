#include "LIB_assert.h"

#include "ast/type.h"

#include "context.h"
#include "token.h"

ROSE_INLINE bool same_array_type(const struct RCCType *a, const struct RCCType *b) {
	if (!(a->kind == TP_ARRAY && b->kind == TP_ARRAY)) {
		return false;
	}

	const RCCTypeArray *arr1 = &a->tp_array, *arr2 = &b->tp_array;

	if (!(arr1->boundary == arr2->boundary)) {
		return false;
	}
	switch (arr1->boundary) {
		case ARRAY_UNBOUNDED: {
		} break;
		case ARRAY_VLA:
		case ARRAY_VLA_STATIC: {
			if (!((arr1->vla_length == NULL && arr2->vla_length == NULL) || (arr1->vla_length != NULL && arr2->vla_length != NULL))) {
				return false;
			}
		} break;
		case ARRAY_BOUNDED:
		case ARRAY_BOUNDED_STATIC: {
			if (!(RCC_type_array_length(a) == RCC_type_array_length(b))) {
				return false;
			}
		} break;
	}
	return RCC_type_same(arr1->element_type, arr2->element_type);
}

ROSE_INLINE bool compatible_array_type(const struct RCCType *a, const struct RCCType *b) {
	if (!(a->kind == TP_ARRAY && b->kind == TP_ARRAY)) {
		return false;
	}

	const RCCTypeArray *arr1 = &a->tp_array, *arr2 = &b->tp_array;

	if (!RCC_type_compatible(arr1->element_type, arr2->element_type)) {
		return false;
	}
	if (ELEM(arr1->boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC) && ELEM(arr2->boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC)) {
		if (!(RCC_type_array_length(a) == RCC_type_array_length(b))) {
			return false;
		}
	}
	return true;
}

ROSE_INLINE const struct RCCType *composite_array_type(struct RCContext *C, const struct RCCType *a, const struct RCCType *b) {
	if (!(a->kind == TP_ARRAY && b->kind == TP_ARRAY)) {
		return NULL;
	}

	const RCCTypeArray *arr1 = &a->tp_array, *arr2 = &b->tp_array;

	if (!RCC_type_compatible(a, b)) {
		return false;
	}
	// Constant Length Array
	if (ELEM(arr1->boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC)) {
		const struct RCCType *element_type = RCC_type_composite(C, arr1->element_type, arr2->element_type);
		const struct RCCNodeConstexpr *const_length = RCC_node_new_const_integer(C, RCC_type_array_length(a));
		return RCC_type_new_array(C, element_type, const_length, NULL);
	}
	if (ELEM(arr2->boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC)) {
		const struct RCCType *element_type = RCC_type_composite(C, arr1->element_type, arr2->element_type);
		const struct RCCNodeConstexpr *const_length = RCC_node_new_const_integer(C, RCC_type_array_length(b));
		return RCC_type_new_array(C, element_type, const_length, NULL);
	}
	// Variable Length Array
	if (ELEM(arr1->boundary, ARRAY_VLA, ARRAY_VLA_STATIC)) {
		const struct RCCType *element_type = RCC_type_composite(C, arr1->element_type, arr2->element_type);
		const struct RCCNode *vla_length = arr1->vla_length;
		return RCC_type_new_vla_array(C, element_type, vla_length, NULL);
	}
	if (ELEM(arr2->boundary, ARRAY_VLA, ARRAY_VLA_STATIC)) {
		const struct RCCType *element_type = RCC_type_composite(C, arr1->element_type, arr2->element_type);
		const struct RCCNode *vla_length = arr2->vla_length;
		return RCC_type_new_vla_array(C, element_type, vla_length, NULL);
	}
	// Unbounded Array
	const struct RCCType *element_type = RCC_type_composite(C, arr1->element_type, arr2->element_type);
	return RCC_type_new_unbounded_array(C, element_type, NULL);
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

ROSE_INLINE RCCType *array_new(struct RCContext *C, int boundary, const RCCType *elem, const RCCTypeQualification *qual) {
	RCCType *a = RCC_context_calloc(C, sizeof(RCCType));

	a->kind = TP_ARRAY;
	a->same = same_array_type;
	a->compatible = compatible_array_type;
	a->composite = composite_array_type;

	a->tp_array.boundary = boundary;
	a->tp_array.element_type = elem;

	memcpy(&a->tp_array.qualification, qual, sizeof(RCCTypeQualification));

	return a;
}

RCCType *RCC_type_new_unbounded_array(struct RCContext *C, const RCCType *elem, const RCCTypeQualification *qual) {
	RCCType *a = array_new(C, ARRAY_UNBOUNDED, elem, qual);

	return a;
}
RCCType *RCC_type_new_array(struct RCContext *C, const RCCType *elem, const struct RCCNodeConstexpr *length, const RCCTypeQualification *qual) {
	RCCType *a = array_new(C, ARRAY_BOUNDED, elem, qual);

	a->tp_array.const_length = length;

	return a;
}
RCCType *RCC_type_new_array_static(struct RCContext *C, const RCCType *elem, const struct RCCNodeConstexpr *length, const RCCTypeQualification *qual) {
	RCCType *a = array_new(C, ARRAY_BOUNDED_STATIC, elem, qual);

	a->tp_array.const_length = length;

	return a;
}
RCCType *RCC_type_new_vla_array(struct RCContext *C, const RCCType *elem, const struct RCCNode *length, const RCCTypeQualification *qual) {
	RCCType *a = array_new(C, ARRAY_VLA, elem, qual);

	a->tp_array.vla_length = length;

	return a;
}
RCCType *RCC_type_new_vla_array_static(struct RCContext *C, const RCCType *elem, const struct RCCNode *length, const RCCTypeQualification *qual) {
	RCCType *a = array_new(C, ARRAY_VLA_STATIC, elem, qual);

	a->tp_array.vla_length = length;

	return a;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

size_t RCC_type_array_length(const RCCType *arr) {
	if (arr->kind != TP_ARRAY || arr->tp_array.boundary == ARRAY_UNBOUNDED) {
		return 0;
	}
	ROSE_assert_unreachable();
	return 0;
}

/** \} */
