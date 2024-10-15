#ifndef RT_AST_TYPE_POINTER_H
#define RT_AST_TYPE_POINTER_H

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RCCType *RT_type_new_pointer(struct RCContext *, const struct RCCType *base);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_type_is_pointer(const struct RCCType *type);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_POINTER_H
