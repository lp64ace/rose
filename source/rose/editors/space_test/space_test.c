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

ROSE_INLINE SpaceLink *test_create(const ScrArea *area) {
	SpaceTest *test = MEM_callocN(sizeof(SpaceTest), "SpaceLink::SpaceTest");

	// Header
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceTest::Header");
		LIB_addtail(&test->regionbase, region);
		region->regiontype = RGN_TYPE_HEADER;
		region->alignment = RGN_ALIGN_TOP;
	}
	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceTest::Main");
		LIB_addtail(&test->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;
	}
	test->spacetype = SPACE_USERPREF;

	return (SpaceLink *)test;
}

ROSE_INLINE void test_free(SpaceLink *link) {
}

ROSE_INLINE void test_init(WindowManager *wm, ScrArea *area) {
}

ROSE_INLINE void test_exit(WindowManager *wm, ScrArea *area) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name UserPref Main Region Methods
 * \{ */

 void test_header_region_layout(struct rContext *C, ARegion *region) {
	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "TEST_header"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 0, region->sizey, 0, 0);
		uiLayout *layout = UI_layout_row(root, PIXELSIZE);
		but = uiDefBut(block, UI_BTYPE_PUSH, "Push 1", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_PUSH, "Push 2", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_PUSH, "Push 3", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT);

		UI_block_end(C, block);
	}
}

void test_main_region_layout(struct rContext *C, ARegion *region) {
	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "TEST_main"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 0, region->sizey, 0, PIXELSIZE);

		uiLayout *layout = UI_layout_grid(root, 5);

		int dynamic = ROSE_MAX(PIXELSIZE, region->sizex - (2 * 8 * UI_UNIT_X + 2 * PIXELSIZE));

		uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Head 1", dynamic, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);
		uiButEnableFlag(uiDefBut(block, UI_BTYPE_SEPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
		uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Head 2", 8 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);
		uiButEnableFlag(uiDefBut(block, UI_BTYPE_SEPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
		uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Head 3", 8 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);

		for (int row = 0; row < 16; row++) {
			but = uiDefBut(block, UI_BTYPE_TEXT, "Static", dynamic, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT);
			but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			but = uiDefBut(block, UI_BTYPE_EDIT, "Edit 1", 8 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT);
			but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			but = uiDefBut(block, UI_BTYPE_EDIT, "Edit 2", 8 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT);
		}

		UI_block_end(C, block);
	}
}

/** \} */

void ED_spacetype_test() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::Test");

	st->spaceid = SPACE_TEST;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "SpaceTest");

	st->create = test_create;
	st->free = test_free;
	st->init = test_init;
	st->exit = test_exit;

	// Header
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceTest::ARegionType::Header");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_HEADER;
		art->layout = test_header_region_layout;
		art->draw = ED_region_header_draw;
		art->init = ED_region_header_init;
		art->exit = ED_region_header_exit;
	}
	// Main Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceTest::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->layout = test_main_region_layout;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}

	KER_spacetype_register(st);
}
