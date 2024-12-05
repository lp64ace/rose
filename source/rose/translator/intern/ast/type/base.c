#include "ast/type.h"

#include "RT_context.h"
#include "RT_token.h"

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

/** type.h */

bool RT_type_same(const RTType *a, const RTType *b) {
	if (a == b) {  // This will also handle the case where a is NULL and b is NULL!
		return true;
	}
	return a && b && a->same(a, b);
}
bool RT_type_compatible(const RTType *a, const RTType *b) {
	if (a == b) {  // This will also handle the case where a is NULL and b is NULL!
		return true;
	}
	return a && b && a->compatible(a, b);
}

const RTType *RT_type_composite(struct RTContext *c, const RTType *a, const RTType *b) {
	if (RT_type_same(a, b)) {  // This will save the allocation of a new type.
		return a;
	}
	return (a && b) ? a->composite(c, a, b) : NULL;
}

/** base.h */

bool RT_qual_is_empty(const struct RTTypeQualification *qual) {
	if (qual) {
		if (qual->is_constant || qual->is_restricted || qual->is_volatile || qual->is_atomic) {
			return false;
		}
	}
	return true;
}

void RT_qual_clear(struct RTTypeQualification *qual) {
	memset(qual, 0, sizeof(RTTypeQualification));
}

/** \} */
