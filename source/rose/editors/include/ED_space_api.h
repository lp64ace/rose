#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ED_spacetypes_init();

/* -------------------------------------------------------------------- */
/** \name Calls for registering default spaces
 *
 * Calls for registering default spaces, only called once, from #ED_spacetypes_init
 * \{ */

void ED_spacetype_topbar();
void ED_spacetype_statusbar();
void ED_spacetype_view3d();
 
/* \} */

#ifdef __cplusplus
}
#endif
