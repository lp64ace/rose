#include "LIB_assert.h"

#include "ast/node.h"
#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

ROSE_INLINE bool same_array_type(const struct RTType *a, const struct RTType *b) {
	if (!(a->kind == TP_ARRAY && b->kind == TP_ARRAY)) {
		return false;
	}

	const RTTypeArray *arr1 = &a->tp_array, *arr2 = &b->tp_array;

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
			if (!(RT_type_array_length(a) == RT_type_array_length(b))) {
				return false;
			}
		} break;
	}
	return RT_type_same(arr1->element_type, arr2->element_type);
}

ROSE_INLINE bool compatible_array_type(const struct RTType *a, const struct RTType *b) {
	if (!(a->kind == TP_ARRAY && b->kind == TP_ARRAY)) {
		return false;
	}

	const RTTypeArray *arr1 = &a->tp_array, *arr2 = &b->tp_array;

	if (!RT_type_compatible(arr1->element_type, arr2->element_type)) {
		return false;
	}
	if (ELEM(arr1->boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC) && ELEM(arr2->boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC)) {
		if (!(RT_type_array_length(a) == RT_type_array_length(b))) {
			return false;
		}
	}
	return true;
}

ROSE_INLINE const struct RTType *composite_array_type(struct RTContext *C, const struct RTType *a, const struct RTType *b) {
	if (!(a->kind == TP_ARRAY && b->kind == TP_ARRAY)) {
		return NULL;
	}

	const RTTypeArray *arr1 = &a->tp_array, *arr2 = &b->tp_array;

	if (!RT_type_compatible(a, b)) {
		return false;
	}
	// Constant Length Array
	if (ELEM(arr1->boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC)) {
		const struct RTType *element_type = RT_type_composite(C, arr1->element_type, arr2->element_type);
		return RT_type_new_array(C, element_type, arr1->const_length, NULL);
	}
	if (ELEM(arr2->boundary, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC)) {
		const struct RTType *element_type = RT_type_composite(C, arr1->element_type, arr2->element_type);
		return RT_type_new_array(C, element_type, arr2->const_length, NULL);
	}
	// Variable Length Array
	if (ELEM(arr1->boundary, ARRAY_VLA, ARRAY_VLA_STATIC)) {
		const struct RTType *element_type = RT_type_composite(C, arr1->element_type, arr2->element_type);
		const struct RTNode *vla_length = arr1->vla_length;
		return RT_type_new_vla_array(C, element_type, vla_length, NULL);
	}
	if (ELEM(arr2->boundary, ARRAY_VLA, ARRAY_VLA_STATIC)) {
		const struct RTType *element_type = RT_type_composite(C, arr1->element_type, arr2->element_type);
		const struct RTNode *vla_length = arr2->vla_length;
		return RT_type_new_vla_array(C, element_type, vla_length, NULL);
	}
	// Unbounded Array
	const struct RTType *element_type = RT_type_composite(C, arr1->element_type, arr2->element_type);
	return RT_type_new_unbounded_array(C, element_type, NULL);
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

ROSE_INLINE RTType *array_new(struct RTContext *C, int boundary, const RTType *elem, const RTTypeQualification *qual) {
	RTType *a = RT_context_calloc(C, sizeof(RTType));

	a->kind = TP_ARRAY;
	a->same = same_array_type;
	a->compatible = compatible_array_type;
	a->composite = composite_array_type;

	a->tp_array.boundary = boundary;
	a->tp_array.element_type = elem;

	if (qual) {
		memcpy(&a->tp_array.qualification, qual, sizeof(RTTypeQualification));
	}
	else {
		RT_qual_clear(&a->tp_array.qualification);
	}

	return a;
}

RTType *RT_type_new_empty_array(RTContext *C) {
	return array_new(C, 0, NULL, NULL);
}

RTType *RT_type_new_unbounded_array(struct RTContext *C, const RTType *elem, const RTTypeQualification *qual) {
	RTType *a = array_new(C, ARRAY_UNBOUNDED, elem, qual);

	a->tp_array.length = 0;

	return a;
}
RTType *RT_type_new_array(struct RTContext *C, const RTType *elem, const struct RTNode *length, const RTTypeQualification *qual) {
	RTType *a = array_new(C, ARRAY_BOUNDED, elem, qual);

	a->tp_array.const_length = length;
	a->tp_array.length = RT_node_evaluate_integer(length);

	return a;
}
RTType *RT_type_new_array_static(struct RTContext *C, const RTType *elem, const struct RTNode *length, const RTTypeQualification *qual) {
	RTType *a = array_new(C, ARRAY_BOUNDED_STATIC, elem, qual);

	a->tp_array.const_length = length;
	a->tp_array.length = RT_node_evaluate_integer(length);

	return a;
}
RTType *RT_type_new_vla_array(struct RTContext *C, const RTType *elem, const struct RTNode *length, const RTTypeQualification *qual) {
	RTType *a = array_new(C, ARRAY_VLA, elem, qual);

	a->tp_array.vla_length = length;
	a->tp_array.length = -1;

	return a;
}
RTType *RT_type_new_vla_array_static(struct RTContext *C, const RTType *elem, const struct RTNode *length, const RTTypeQualification *qual) {
	RTType *a = array_new(C, ARRAY_VLA_STATIC, elem, qual);

	a->tp_array.vla_length = length;
	a->tp_array.length = -1;

	return a;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

int RT_type_array_boundary(const struct RTType *arr) {
	ROSE_assert(arr->kind == TP_ARRAY);
	return arr->tp_array.boundary;
}

const struct RTType *RT_type_array_element(const struct RTType *arr) {
	ROSE_assert(arr->kind == TP_ARRAY);
	return arr->tp_array.element_type;
}
unsigned long long RT_type_array_length(const RTType *arr) {
	ROSE_assert(arr->kind == TP_ARRAY);
	return arr->tp_array.length;
}

/** \} */
