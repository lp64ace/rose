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
#include "KER_screen.h"

#include "WM_api.h"
#include "WM_window.h"

/* -------------------------------------------------------------------- */
/** \name File SpaceType Methods
 * \{ */

 ROSE_INLINE SpaceLink *file_create(const ScrArea *area) {
	SpaceFile *file = MEM_callocN(sizeof(SpaceUser), "SpaceFile::SpaceFile");

	// Header Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceFile::Header");
		LIB_addtail(&file->regionbase, region);
		region->regiontype = RGN_TYPE_HEADER;
		region->alignment = RGN_ALIGN_TOP;
	}
	// Footer Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceFile::Footer");
		LIB_addtail(&file->regionbase, region);
		region->regiontype = RGN_TYPE_FOOTER;
		region->alignment = RGN_ALIGN_BOTTOM;
	}
	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceFile::Main");
		LIB_addtail(&file->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;
		region->vscroll = 1;
	}
	file->spacetype = SPACE_FILE;

	return (SpaceLink *)file;
}

ROSE_INLINE void file_free(SpaceLink *link) {
}

ROSE_INLINE void file_init(WindowManager *wm, ScrArea *area) {
}

ROSE_INLINE void file_exit(WindowManager *wm, ScrArea *area) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Header Region Methods
 * \{ */

void file_header_region_layout(struct rContext *C, ARegion *region) {
	ScrArea *area = CTX_wm_area(C);
	SpaceFile *file = (SpaceFile *)area->spacedata.first;

	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "FILE_header"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 0, region->sizey, 0, 0);
		uiLayout *layout = UI_layout_grid(root, 2, true, false);
		int unit = ROSE_MAX(UI_UNIT_X, region->sizex - UI_UNIT_X);
		but = uiDefBut(block, UI_BTYPE_PUSH, "\u2B06\uFE0F", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, 0);
		but = uiDefBut(block, UI_BTYPE_EDIT, "(path)", 0, 0, unit, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT);
		UI_block_end(C, block);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Main Region Methods
 * \{ */

void file_main_region_layout(struct rContext *C, ARegion *region) {
	ScrArea *area = CTX_wm_area(C);
	SpaceFile *file = (SpaceFile *)area->spacedata.first;

	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "FILE_file_listview"))) {
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Footer Region Methods
 * \{ */

 void file_footer_region_layout(struct rContext *C, ARegion *region) {
	ScrArea *area = CTX_wm_area(C);
	SpaceFile *file = (SpaceFile *)area->spacedata.first;

	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "FILE_header"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 0, region->sizey, 0, 0);
		uiLayout *layout = UI_layout_row(root, PIXELSIZE);
		int flex = ROSE_MAX(0, region->sizex - 4 * UI_UNIT_X);
		but = uiDefBut(block, UI_BTYPE_TEXT, "(selected)", 0, 0, flex, UI_UNIT_Y, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_PUSH, "OK", 0, 0, 2 * UI_UNIT_X, UI_UNIT_Y, NULL, 0, 0, 0);
		but = uiDefBut(block, UI_BTYPE_PUSH, "CANCEL", 0, 0, 2 * UI_UNIT_X, UI_UNIT_Y, NULL, 0, 0, 0);
		UI_block_end(C, block);
	}
}

/** \} */

void ED_spacetype_file() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::File");

	st->spaceid = SPACE_FILE;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "File");

	st->create = file_create;
	st->free = file_free;
	st->init = file_init;
	st->exit = file_exit;

	// Header
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceUser::ARegionType::Header");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_HEADER;
		art->layout = file_header_region_layout;
		art->draw = ED_region_header_draw;
		art->init = ED_region_header_init;
		art->exit = ED_region_header_exit;
	}
	// Footer Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceUser::ARegionType::Footer");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_FOOTER;
		art->layout = file_footer_region_layout;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}
	// Main Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceUser::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->layout = file_main_region_layout;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}

	KER_spacetype_register(st);
}
