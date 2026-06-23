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

/* -------------------------------------------------------------------- */
/** \name Drawing Utility Functions
 * \{ */

void ED_view3d_draw_setup_view(struct RegionView3D *rv3d);

/** \} */

#ifdef __cplusplus
}
#endif

#endif /* VIEW3D_INTERN_H */
