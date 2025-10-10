#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "UI_interface.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "WM_api.h"

/* -------------------------------------------------------------------- */
/** \name StatusBar SpaceType Methods
 * \{ */

ROSE_INLINE SpaceLink *statusbar_create(const ScrArea *area) {
	SpaceStatusBar *statusbar = MEM_callocN(sizeof(SpaceStatusBar), "SpaceLink::SpaceStatusBar");

	// Header
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceStatusBar::Header");
		LIB_addtail(&statusbar->regionbase, region);
		region->regiontype = RGN_TYPE_HEADER;
		region->alignment = RGN_ALIGN_BOTTOM;

		region->flag |= RGN_FLAG_ALWAYS_REDRAW;
	}
	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceStatusBar::Main");
		LIB_addtail(&statusbar->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;
	}
	statusbar->spacetype = SPACE_STATUSBAR;

	return (SpaceLink *)statusbar;
}

ROSE_INLINE void statusbar_free(SpaceLink *link) {
}

ROSE_INLINE void statusbar_init(WindowManager *wm, ScrArea *area) {
}

ROSE_INLINE void statusbar_exit(WindowManager *wm, ScrArea *area) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name StatusBar Main Region Methods
 * \{ */

ROSE_STATIC void statusbar_header_region_layout(struct rContext *C, ARegion *region) {
	wmWindow *window = CTX_wm_window(C);

	char memory[64];
	LIB_strnformat_byte_size(memory, ARRAY_SIZE(memory), MEM_num_memory_in_use(), 1);

	uiBlock *block;
	if ((block = UI_block_begin(C, region, "statusbar"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 1, region->sizey, 0, 1);
		uiDefBut(block, UI_BTYPE_TEXT, "(null)", 0, 0, 3 * UI_UNIT_X, UI_UNIT_Y, &window->average_fps, UI_POINTER_DBL, 16, UI_BUT_TEXT_LEFT);
		UI_block_end(C, block);
	}
}

ROSE_STATIC void statusbar_header_region_draw(struct rContext *C, ARegion *region) {
	ED_region_header_draw(C, region);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name StatusBar Main Region Methods
 * \{ */

/** \} */

void ED_spacetype_statusbar() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::StatusBar");

	st->spaceid = SPACE_STATUSBAR;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "StatusBar");

	st->create = statusbar_create;
	st->free = statusbar_free;
	st->init = statusbar_init;
	st->exit = statusbar_exit;

	// Header
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "StatusBar::ARegionType::Header");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_HEADER;
		art->layout = statusbar_header_region_layout;
		art->draw = statusbar_header_region_draw;
		art->init = ED_region_header_init;
		art->exit = ED_region_header_exit;
	}
	// Main
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "StatusBar::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}

	KER_spacetype_register(st);
}
