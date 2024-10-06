#ifndef __RCC_TYPE_QUALIFIED_H__
#define __RCC_TYPE_QUALIFIED_H__

#include "LIB_utildefines.h"

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;
struct RCCType;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCTypeQualified {
	const struct RCCType *base;

	struct RCCTypeQualification qualification;
} RCCTypeQualified;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RCCType *RCC_type_new_qualified(struct RCContext *, const struct RCCType *type);
struct RCCType *RCC_type_new_qualified_ex(struct RCContext *, const struct RCCType *type, const RCCTypeQualification *qualification);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RCC_type_qualified_const(struct RCCType *qual);
void RCC_type_qualified_restrict(struct RCCType *qual);
void RCC_type_qualified_volatile(struct RCCType *qual);
void RCC_type_qualified_atomic(struct RCCType *qual);

bool RCC_type_qualified_is_const(const struct RCCType *qual);
bool RCC_type_qualified_is_restrict(const struct RCCType *qual);
bool RCC_type_qualified_is_volatile(const struct RCCType *qual);
bool RCC_type_qualified_is_atomic(const struct RCCType *qual);

const struct RCCType *RCC_type_unqualified(const struct RCCType *type);
const struct RCCTypeQualification *RCC_type_qualification(const struct RCCType *type);

bool RCC_type_is_zero_qualified(const struct RCCType *type);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_TYPE_QUALIFIED_H__
