#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_screen.h"

/* -------------------------------------------------------------------- */
/** \name TopBar SpaceType Methods
 * \{ */

ROSE_INLINE SpaceLink *topbar_create(const ScrArea *area) {
	SpaceTopBar *topbar = MEM_callocN(sizeof(SpaceTopBar), "SpaceLink::SpaceTopBar");

	// Left Header
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceTopBar::Left");
		LIB_addtail(&topbar->regionbase, region);
		region->regiontype = RGN_TYPE_HEADER;
		region->alignment = RGN_ALIGN_TOP;
	}
	// Right Header
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceTopBar::Right");
		LIB_addtail(&topbar->regionbase, region);
		region->regiontype = RGN_TYPE_HEADER;
		region->alignment = RGN_ALIGN_RIGHT | RGN_SPLIT_PREV;
	}
	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceTopBar::Main");
		LIB_addtail(&topbar->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;
	}

	return (SpaceLink *)topbar;
}

ROSE_INLINE void topbar_free(SpaceLink *link) {
}

ROSE_INLINE void topbar_init(WindowManager *wm, ScrArea *area) {
}

ROSE_INLINE void topbar_exit(WindowManager *wm, ScrArea *area) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name TopBar Main Region Methods
 * \{ */

void topbar_main_region_draw(struct rContext *C, ARegion *region) {
}

void topbar_main_region_init(ARegion *region) {
}
void topbar_main_region_exit(ARegion *region) {
}

/** \} */

void ED_spacetype_topbar() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::TopBar");

	st->spaceid = SPACE_TOPBAR;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "TopBar");

	st->create = topbar_create;
	st->free = topbar_free;
	st->init = topbar_init;
	st->exit = topbar_exit;

	// Header
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceTopBar::ARegionType::Header");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_HEADER;
		art->draw = ED_region_header_draw;
		art->init = ED_region_header_init;
		art->exit = ED_region_header_exit;
	}
	// Main Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceTopBar::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->draw = topbar_main_region_draw;
		art->init = topbar_main_region_init;
		art->exit = topbar_main_region_exit;
	}

	KER_spacetype_register(st);
}
