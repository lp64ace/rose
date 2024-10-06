#ifndef __RCC_TYPE_ARRAY_H__
#define __RCC_TYPE_ARRAY_H__

#include "LIB_utildefines.h"

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCTypeArray {
	const struct RCCType *element_type;
	int boundary;

	struct RCCTypeQualification qualification;

	union {
		const struct RCCNodeConstexpr *const_length;
		const struct RCCNode *vla_length;
	};
} RCCTypeArray;

enum { ARRAY_UNBOUNDED, ARRAY_BOUNDED, ARRAY_BOUNDED_STATIC, ARRAY_VLA, ARRAY_VLA_STATIC };

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RCCType *RCC_type_new_unbounded_array(struct RCContext *, const struct RCCType *elem, const RCCTypeQualification *qual);
struct RCCType *RCC_type_new_array(struct RCContext *, const struct RCCType *elem, const struct RCCNodeConstexpr *length, const RCCTypeQualification *qual);
struct RCCType *RCC_type_new_array_static(struct RCContext *, const struct RCCType *elem, const struct RCCNodeConstexpr *length, const RCCTypeQualification *qual);
struct RCCType *RCC_type_new_vla_array(struct RCContext *, const struct RCCType *elem, const struct RCCNode *length, const RCCTypeQualification *qual);
struct RCCType *RCC_type_new_vla_array_static(struct RCContext *, const struct RCCType *elem, const struct RCCNode *length, const RCCTypeQualification *qual);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

size_t RCC_type_array_length(const struct RCCType *arr);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_TYPE_ARRAY_H__
