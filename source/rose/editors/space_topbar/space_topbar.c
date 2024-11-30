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
/** \name TopBar SpaceType Methods
 * \{ */

ROSE_INLINE SpaceLink *topbar_create(const ScrArea *area) {
	SpaceTopBar *topbar = MEM_callocN(sizeof(SpaceTopBar), "SpaceLink::SpaceTopBar");

	// Header
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceTopBar::Header");
		LIB_addtail(&topbar->regionbase, region);
		region->regiontype = RGN_TYPE_HEADER;
		region->alignment = RGN_ALIGN_TOP;
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
/** \name TopBar Header Region Methods
 * \{ */

ROSE_INLINE void topbar_header_file_menu_but(struct rContext *C, void *vbut, void *unused) {
	uiBut *but = (uiBut *)vbut;
	
	if (STREQ(but->name, "Quit")) {
		wmWindow *window = CTX_wm_window(C);
		
		/** Close the window and any child windows and potentially exit the application. */
		WM_window_close(C, window);
	}
}

ROSE_INLINE uiBlock *topbar_header_file_menu(struct rContext *C, ARegion *region, void *arg) {
	const int isize_x = 6 * PIXELSIZE * UI_UNIT_X;
	const int isize_y = 1 * PIXELSIZE * UI_UNIT_Y;
	
	uiBlock *block;
	if ((block = UI_block_begin(C, region, "file"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_VERTICAL, ITEM_LAYOUT_ROOT, 0, 0, 0, 0);
		uiDefBut(block, UI_BTYPE_BUT, "New", 0, 0, isize_x, isize_y, topbar_header_file_menu_but);
		uiDefBut(block, UI_BTYPE_BUT, "Open", 0, 0, isize_x, isize_y, topbar_header_file_menu_but);
		uiDefSepr(block, UI_BTYPE_HSEPR, "", 0, 0, isize_x, 1);
		
		uiDefBut(block, UI_BTYPE_BUT, "Save", 0, 0, isize_x, isize_y, topbar_header_file_menu_but);
		uiDefBut(block, UI_BTYPE_BUT, "Save As...", 0, 0, isize_x, isize_y, topbar_header_file_menu_but);
		uiDefSepr(block, UI_BTYPE_HSEPR, "", 0, 0, isize_x, 1);
		
		uiDefBut(block, UI_BTYPE_BUT, "Quit", 0, 0, isize_x, isize_y, topbar_header_file_menu_but);
		uiDefSepr(block, UI_BTYPE_HSEPR, "", 0, 0, isize_x, 1);
		UI_block_end(C, block);
	}
	return block;
}

ROSE_INLINE void topbar_header_region_layout(struct rContext *C, ARegion *region) {
	uiBlock *block;
	if ((block = UI_block_begin(C, region, "topbar"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 0, region->sizey, 0, 0);
		uiLayout *layout = UI_layout_row(root, 1);
		uiDefMenu(block, UI_BTYPE_MENU, "File", 0, 0, 2 * UI_UNIT_X, region->sizey, topbar_header_file_menu);
		UI_block_end(C, block);
	}
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
		art->layout = topbar_header_region_layout;
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
