#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "UI_interface.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "RLO_readfile.h"
#include "RLO_writefile.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "WM_api.h"
#include "WM_window.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name UserPref SpaceType Methods
 * \{ */

ROSE_INLINE SpaceLink *user_create(const ScrArea *area) {
	SpaceUser *user = MEM_callocN(sizeof(SpaceUser), "SpaceLink::SpaceUser");

	// Header
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceUser::Header");
		LIB_addtail(&user->regionbase, region);
		region->regiontype = RGN_TYPE_HEADER;
		region->alignment = RGN_ALIGN_TOP;
	}
	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceUser::Main");
		LIB_addtail(&user->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;
	}
	user->spacetype = SPACE_USERPREF;

	return (SpaceLink *)user;
}

ROSE_INLINE void user_free(SpaceLink *link) {
}

ROSE_INLINE void user_init(WindowManager *wm, ScrArea *area) {
}

ROSE_INLINE void user_exit(WindowManager *wm, ScrArea *area) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name UserPref Main Region Methods
 * \{ */

void user_main_region_layout(struct rContext *C, ARegion *region) {
	uiBlock *block;
	if ((block = UI_block_begin(C, region, "statusbar"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 1, region->sizey, 0, 1);

		UI_layout_grid(root, 3);

		uiBut *but;
		but = uiDefBut(block, UI_BTYPE_TEXT, "First", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiButEnableFlag(but, UI_DISABLED);
		but = uiDefBut(block, UI_BTYPE_TEXT, "Last", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiButEnableFlag(but, UI_DISABLED);
		but = uiDefBut(block, UI_BTYPE_TEXT, "Email", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiButEnableFlag(but, UI_DISABLED);

		uiDefBut(block, UI_BTYPE_TEXT, "Dimitris", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_TEXT, "Bokis", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_TEXT, "bokisdimitris@gmail.com", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);

		uiDefBut(block, UI_BTYPE_TEXT, "George", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_TEXT, "Ypsilantis", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_TEXT, "unkown@gmail.com", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);

		UI_block_layout_set_current(block, root);

		uiDefBut(block, UI_BTYPE_SEPR, "Hidden", 1 * UI_UNIT_X, 8 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);

		uiDefBut(block, UI_BTYPE_TEXT, "Static", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_PUSH, "Button", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_EDIT, "Edit 1", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_EDIT, "Edit 2", 8 * UI_UNIT_X, 1 * UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);

		UI_block_end(C, block);
	}
}

/** \} */

void ED_spacetype_userpref() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::UserPref");

	st->spaceid = SPACE_USERPREF;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "UserPref");

	st->create = user_create;
	st->free = user_free;
	st->init = user_init;
	st->exit = user_exit;

	// Header
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceUser::ARegionType::Header");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_HEADER;
		art->draw = ED_region_header_draw;
		art->init = ED_region_header_init;
		art->exit = ED_region_header_exit;
	}
	// Main Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceUser::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->layout = user_main_region_layout;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}

	KER_spacetype_register(st);
}
