#include "ED_space_api.h"

#include "KER_screen.h"

void ED_spacetypes_init() {
	ED_spacetype_debug();
	ED_spacetype_statusbar();
	ED_spacetype_topbar();
	ED_spacetype_view3d();
	ED_spacetype_userpref();
	ED_spacetype_file();
}

void ED_spacetypes_exit() {
	/** Free all the spacetypes that were registered! */
	KER_spacetype_free();
}
