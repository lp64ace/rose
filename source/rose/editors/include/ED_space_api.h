#ifndef ED_SPACE_API_H
#define ED_SPACE_API_H

#ifdef __cplusplus
extern "C" {
#endif

void ED_spacetypes_init();
void ED_spacetypes_exit();

/* -------------------------------------------------------------------- */
/** \name Calls for registering default spaces
 *
 * Calls for registering default spaces, only called once, from #ED_spacetypes_init
 * \{ */

void ED_spacetype_statusbar();
void ED_spacetype_topbar();
void ED_spacetype_view3d();
void ED_spacetype_properties();

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// ED_SPACE_API_H
