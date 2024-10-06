#include "ast/type.h"

#include "context.h"
#include "token.h"

ROSE_INLINE bool same_pointer(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_PTR && b->kind == TP_PTR)) {
		return false;
	}
	return RCC_type_same(a->tp_base, b->tp_base);
}

ROSE_INLINE bool compatible_pointer(const RCCType *a, const RCCType *b) {
	if (!(a->kind == TP_PTR && b->kind == TP_PTR)) {
		return false;
	}
	return RCC_type_compatible(a->tp_base, b->tp_base);
}

ROSE_INLINE const RCCType *composite_pointer(struct RCContext *C, const RCCType *a, const RCCType *b) {
	if (!RCC_type_compatible(a, b)) {
		return NULL;
	}
	return RCC_type_new_pointer(C, RCC_type_composite(C, a->tp_base, b->tp_base));
}

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

RCCType *RCC_type_new_pointer(struct RCContext *C, const RCCType *base) {
	RCCType *p = RCC_context_calloc(C, sizeof(RCCType));

	p->kind = TP_PTR;
	p->same = same_pointer;
	p->compatible = compatible_pointer;
	p->composite = composite_pointer;

	p->tp_base = base;

	return p;
}

/** \} */
