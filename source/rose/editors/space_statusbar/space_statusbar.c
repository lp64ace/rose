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
	}
	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceStatusBar::Main");
		LIB_addtail(&statusbar->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;
	}

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

void statusbar_main_region_draw(struct rContext *C, ARegion *region) {
}

void statusbar_main_region_init(ARegion *region) {
}
void statusbar_main_region_exit(ARegion *region) {
}

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
		art->draw = ED_region_header_draw;
		art->init = ED_region_header_init;
		art->exit = ED_region_header_exit;
	}
	// Main
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "StatusBar::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->draw = statusbar_main_region_draw;
		art->init = statusbar_main_region_init;
		art->exit = statusbar_main_region_exit;
	}

	KER_spacetype_register(st);
}