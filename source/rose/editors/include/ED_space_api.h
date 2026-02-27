#ifndef ED_SPACE_API_H
#define ED_SPACE_API_H

#include <stdbool.h>

struct rContext;

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

void ED_spacetype_file();
void ED_spacetype_statusbar();
void ED_spacetype_topbar();
void ED_spacetype_view3d();
void ED_spacetype_properties();

/** \} */

/* -------------------------------------------------------------------- */
/** \name Helper Routines
 * \{ */

bool ED_operator_file_browsing_active(struct rContext *C);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// ED_SPACE_API_H
