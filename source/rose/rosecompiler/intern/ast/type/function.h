#ifndef __RCC_TYPE_FUNCTION_H__
#define __RCC_TYPE_FUNCTION_H__

#include "LIB_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;
struct RCCToken;
struct RCCType;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCCTypeFunction {
	const struct RCCType *return_type;

	ListBase parameters;
} RCCTypeFunction;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCType *RCC_type_new_function(struct RCContext *, const struct RCCType *ret);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RCC_type_function_add_named_parameter(struct RCContext *, struct RCCType *f, const struct RCCType *type, const struct RCCToken *name);
void RCC_type_function_add_parameter(struct RCContext *, struct RCCType *f, const struct RCCType *type);
void RCC_type_function_add_ellipsis_parameter(struct RCContext *, struct RCCType *f);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_TYPE_FUNCTION_H__
