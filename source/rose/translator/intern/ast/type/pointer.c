#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

ROSE_INLINE bool same_pointer(const RTType *a, const RTType *b) {
	if (!(a->kind == TP_PTR && b->kind == TP_PTR)) {
		return false;
	}
	return RT_type_same(a->tp_base, b->tp_base);
}

ROSE_INLINE bool compatible_pointer(const RTType *a, const RTType *b) {
	if (!(a->kind == TP_PTR && b->kind == TP_PTR)) {
		return false;
	}
	return RT_type_compatible(a->tp_base, b->tp_base);
}

ROSE_INLINE const RTType *composite_pointer(struct RTContext *C, const RTType *a, const RTType *b) {
	if (!RT_type_compatible(a, b)) {
		return NULL;
	}
	return RT_type_new_pointer(C, RT_type_composite(C, a->tp_base, b->tp_base));
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

RTType *RT_type_new_pointer(struct RTContext *C, const RTType *base) {
	RTType *p = RT_context_calloc(C, sizeof(RTType));

	p->kind = TP_PTR;
	p->same = same_pointer;
	p->compatible = compatible_pointer;
	p->composite = composite_pointer;

	p->tp_base = base;

	return p;
}

RTType *RT_type_new_empty_pointer(RTContext *C) {
	return RT_type_new_pointer(C, NULL);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_type_is_pointer(const RTType *type) {
	return type->kind == TP_PTR;
}

/** \} */
