#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"
#include "DNA_userdef_types.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "UI_interface.h"
#include "UI_resource.h"
#include "UI_view2d.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RLO_readfile.h"
#include "RLO_writefile.h"

#include "KER_context.h"
#include "KER_idtype.h"
#include "KER_main.h"
#include "KER_screen.h"

#include "WM_api.h"
#include "WM_window.h"

/* -------------------------------------------------------------------- */
/** \name File SpaceType Methods
 * \{ */

ROSE_INLINE SpaceLink *debug_create(const ScrArea *area) {
	SpaceDebug *debug = MEM_callocN(sizeof(SpaceDebug), "SpaceDebug::SpaceDebug");

	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceDebug::Main");
		LIB_addtail(&debug->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;
		// region->flag |= RGN_FLAG_ALWAYS_REBUILD;
		region->vscroll = 1;
	}
	debug->spacetype = SPACE_DEBUG;

	return (SpaceLink *)debug;
}

ROSE_INLINE void debug_free(SpaceLink *link) {
}

ROSE_INLINE void debug_init(WindowManager *wm, ScrArea *area) {
}

ROSE_INLINE void debug_exit(WindowManager *wm, ScrArea *area) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Debug SpaceType Methods
 * \{ */

static unsigned int selected = 0;
static const char *options[] = {
	"DIRECTX9",
	"DIRECTX10",
	"DIRECTX11",
	"DIRECTX12",
	"OPENGL",
};

ROSE_STATIC void debug_main_region_layout(struct rContext *C, ARegion *region) {
	Main *main = CTX_data_main(C);
	ScrArea *area = CTX_wm_area(C);
	SpaceDebug *debug = (SpaceDebug *)area->spacedata.first;

	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "DEBUG_datablock_listview"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, region->sizex * region->hscroll, region->sizey * region->vscroll, 0, 0);

		UI_block_end(C, block);
	}
}

/** \} */

void ED_spacetype_debug() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::Debug");

	st->spaceid = SPACE_DEBUG;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "Debug");

	st->create = debug_create;
	st->free = debug_free;
	st->init = debug_init;
	st->exit = debug_exit;

	// Main Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceDebug::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->layout = debug_main_region_layout;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}

	KER_spacetype_register(st);
}
