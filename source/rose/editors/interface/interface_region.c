#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_windowmanager_types.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "ED_screen.h"

#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_string_utf.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "interface_intern.h"

#include <stdio.h>

ARegion *ui_region_temp_add(Screen *screen) {
	ARegion *region = MEM_callocN(sizeof(ARegion), "ARegion");
	LIB_addhead(&screen->regionbase, region);

	region->regiontype = RGN_TYPE_TEMPORARY;
	region->alignment = RGN_ALIGN_FLOAT;

	ED_region_tag_redraw(region);

	return region;
}

void ui_region_temp_remove(rContext *C, Screen *screen, ARegion *region) {
	wmWindow *win = CTX_wm_window(C);

	ROSE_assert(region->regiontype == RGN_TYPE_TEMPORARY);
	ROSE_assert(LIB_haslink(&screen->regionbase, region));

	ED_region_exit(C, region);
	KER_area_region_free(NULL, region);
	LIB_remlink(&screen->regionbase, region);
	MEM_freeN(region);

	if (CTX_wm_region(C) == region) {
		CTX_wm_region_set(C, NULL);
	}
}
