#ifndef __RCC_TYPE_POINTER_H__
#define __RCC_TYPE_POINTER_H__

#ifdef __cplusplus
extern "C" {
#endif

struct RCContext;

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RCCType *RCC_type_new_pointer(struct RCContext *, const struct RCCType *base);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// __RCC_TYPE_POINTER_H__
