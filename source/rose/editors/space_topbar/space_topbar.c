#include "ED_screen.h"
#include "ED_space_api.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_math.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "MEM_alloc.h"

#include "WM_api.h"
#include "WM_handlers.h"
#include "WM_types.h"
#include "WM_window.h"

#include "UI_interface.h"
#include "UI_view2d.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "RFT_api.h"

#include <stdio.h>

static SpaceLink *topbar_create(const struct ScrArea *area) {
	ARegion *region;
	SpaceTopBar *topbar;

	topbar = MEM_callocN(sizeof(SpaceTopBar), "TopBar::Link");
	topbar->spacetype = SPACE_TOPBAR;

	region = MEM_callocN(sizeof(ARegion), "TopBar::header (left)");
	LIB_addtail(&topbar->regionbase, region);
	region->regiontype = RGN_TYPE_HEADER;
	region->alignment = RGN_ALIGN_TOP;

	region = MEM_callocN(sizeof(ARegion), "TopBar::header (right)");
	LIB_addtail(&topbar->regionbase, region);
	region->regiontype = RGN_TYPE_HEADER;
	region->alignment = RGN_ALIGN_RIGHT | RGN_SPLIT_PREV;

	region = MEM_callocN(sizeof(ARegion), "TopBar::main");
	LIB_addtail(&topbar->regionbase, region);
	region->regiontype = RGN_TYPE_WINDOW;

	return (SpaceLink *)topbar;
}

static void topbar_free(const struct SpaceLink *link) {
}

static void topbar_init(const struct wmWindowManager *wm, struct ScrArea *area) {
}

static SpaceLink *topbar_duplicate(const struct SpaceLink *sl) {
	SpaceTopBar *topbar = MEM_dupallocN(sl);

	return (SpaceLink *)topbar;
}

static void topbar_header_region_init(struct wmWindowManager *wm, struct ARegion *region) {
	if (RGN_ALIGN_ENUM_FROM_MASK(region->alignment) == RGN_ALIGN_RIGHT) {
		region->flag |= RGN_FLAG_DYNAMIC_SIZE;
	}
	ED_region_header_init(region);
}

static void topbar_main_region_init(struct wmWindowManager *wm, struct ARegion *region) {
	/** force delayed UI_view2d_region_reinit call */
	if (ELEM(RGN_ALIGN_ENUM_FROM_MASK(region->alignment), RGN_ALIGN_RIGHT)) {
		region->flag |= RGN_FLAG_DYNAMIC_SIZE;
	}
	UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_HEADER, region->winx, region->winy);
}

static void topbar_window_title_draw(const struct Context *C, struct ARegion *region) {
	int height = region->winy;
	const char *title_text = "Rose";
	
	int fontid = RFT_default();
	RFT_color3f(fontid, 1.0f, 1.0f, 1.0f);
	RFT_size(fontid, (float)(height >> 1));
	RFT_position(fontid, 8, 8, 0);
	
	RFT_draw(fontid, title_text, ARRAY_SIZE(title_text));
}

static void topbar_main_region_draw(const struct Context *C, struct ARegion *region) {
	ED_region_header_draw(C, region);
	topbar_window_title_draw(C, region);
}

void ED_spacetype_topbar() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "rose::spacetype::TopBar");

	ARegionType *art;

	st->spaceid = SPACE_TOPBAR;
	LIB_strncpy(st->name, "Top Bar", ARRAY_SIZE(st->name));

	st->create = topbar_create;
	st->free = topbar_free;
	st->init = topbar_init;
	st->duplicate = topbar_duplicate;

	art = MEM_callocN(sizeof(ARegionType), "rose::regiontype::TopBar::MainRegion");
	art->regionid = RGN_TYPE_WINDOW;
	art->init = topbar_main_region_init;
	art->draw = topbar_main_region_draw;
	art->prefsizex = UI_UNIT_X * 8;

	LIB_addhead(&st->regiontypes, art);

	art = MEM_callocN(sizeof(ARegionType), "rose::regiontype::TopBar::HeaderRegion");
	art->regionid = RGN_TYPE_HEADER;
	art->init = topbar_header_region_init;
	art->draw = ED_region_header_draw;
	art->prefsizex = UI_UNIT_X * 8;
	art->prefsizey = HEADERY;

	LIB_addhead(&st->regiontypes, art);

	KER_spacetype_register(st);
}
