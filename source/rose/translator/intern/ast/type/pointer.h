#ifndef RT_AST_TYPE_POINTER_H
#define RT_AST_TYPE_POINTER_H

#ifdef __cplusplus
extern "C" {
#endif

struct RTContext;

/* -------------------------------------------------------------------- */
/** \name Creation Methods
 * \{ */

struct RTType *RT_type_new_empty_pointer(struct RTContext *);

struct RTType *RT_type_new_pointer(struct RTContext *, const struct RTType *base);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

bool RT_type_is_pointer(const struct RTType *type);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_AST_TYPE_POINTER_H
