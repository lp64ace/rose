#include "ED_space_api.h"

#include "KER_screen.h"

void ED_spacetypes_init() {
	ED_spacetype_statusbar();
	ED_spacetype_topbar();
	ED_spacetype_view3d();
}

void ED_spacetypes_exit() {
	/** Free all the spacetypes that were registered! */
	KER_spacetype_free();
}
