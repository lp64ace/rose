#ifndef VIEW3D_INTERN_H
#define VIEW3D_INTERN_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Assigning Operator Types
 * \{ */

void view3d_operatortypes();

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operator Key Map
 * \{ */

void view3d_keymap(struct wmKeyConfig *keyconf);

/** \} */

#ifdef __cplusplus
}
#endif

#endif /* VIEW3D_INTERN_H */
