#ifndef RT_AST_TYPE_FUNCTION_H
#define RT_AST_TYPE_FUNCTION_H

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

typedef struct RCCTypeParameter {
	struct RCCTypeParameter *prev, *next;

	const struct RCCToken *name;
	const struct RCCType *type;
	const struct RCCType *adjusted;
} RCCTypeParameter;

typedef struct RCCTypeFunction {
	const struct RCCType *return_type;

	unsigned int is_variadic : 1;
	unsigned int is_complete : 1;

	ListBase parameters;
} RCCTypeFunction;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

struct RCCType *RT_type_new_function(struct RCContext *, const struct RCCType *ret);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void RT_type_function_add_named_parameter(struct RCContext *, struct RCCType *f, const struct RCCType *type, const struct RCCToken *name);
void RT_type_function_add_parameter(struct RCContext *, struct RCCType *f, const struct RCCType *type);
void RT_type_function_add_ellipsis_parameter(struct RCContext *, struct RCCType *f);

void RT_type_function_finalize(struct RCContext *, struct RCCType *f);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_FUNCTION_H
