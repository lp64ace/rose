#ifndef VIEW3D_INTERN_H
#define VIEW3D_INTERN_H

struct ARegion;
struct RegionView3D;
struct wmKeyConfig;

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

struct RegionView3D *ED_view3d_region_view_init(struct ARegion *v3d, struct RegionView3D *rv3d);
void ED_view3d_draw_setup_view(struct ARegion *v3d, const float viewmat[4][4], const float winmat[4][4], struct rcti *rect);

/** \} */

#ifdef __cplusplus
}
#endif

#endif /* VIEW3D_INTERN_H */
