#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

ROSE_INLINE bool same_pointer(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_PTR && b->kind == TP_PTR)) {
		return false;
	}
	return RT_type_same(a->tp_base, b->tp_base);
}

ROSE_INLINE bool compatible_pointer(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_PTR && b->kind == TP_PTR)) {
		return false;
	}
	return RT_type_compatible(a->tp_base, b->tp_base);
}

ROSE_INLINE const RCCType *composite_pointer(struct RCContext *C, const RCCType *a, const RCCType *b) {
	if (!RT_type_compatible(a, b)) {
		return NULL;
	}
	return RT_type_new_pointer(C, RT_type_composite(C, a->tp_base, b->tp_base));
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

RCCType *RT_type_new_pointer(struct RCContext *C, const RCCType *base) {
	RCCType *p = RT_context_calloc(C, sizeof(RCCType));

	p->kind = TP_PTR;
	p->same = same_pointer;
	p->compatible = compatible_pointer;
	p->composite = composite_pointer;

	p->tp_base = base;

	return p;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_type_is_pointer(const RCCType *type) {
	return type->kind == TP_PTR;
}

/** \} */
