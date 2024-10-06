#include "ast/type.h"

#include "context.h"
#include "token.h"

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

/** type.h */

bool RCC_type_same(const RCCType *a, const RCCType *b) {
	if (a == b) {  // This will also handle the case where a is NULL and b is NULL!
		return true;
	}
	return a->same(a, b);
}
bool RCC_type_compatible(const RCCType *a, const RCCType *b) {
	if (a == b) {  // This will also handle the case where a is NULL and b is NULL!
		return true;
	}
	return a->compatible(a, b);
}

const RCCType *RCC_type_composite(struct RCContext *c, const RCCType *a, const RCCType *b) {
	if (RCC_type_same(a, b)) {	// This will save the allocation of a new type.
		return a;
	}
	return a->composite(c, a, b);
}

/** base.h */

bool RCC_qual_is_empty(const struct RCCTypeQualification *qual) {
	if (qual) {
		if (qual->is_constant || qual->is_restricted || qual->is_volatile || qual->is_atomic) {
			return false;
		}
	}
	return true;
}

/** \} */
