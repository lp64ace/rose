#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "GPU_framebuffer.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "UI_interface.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_screen.h"

/* -------------------------------------------------------------------- */
/** \name View3D SpaceType Methods
 * \{ */

ROSE_INLINE SpaceLink *view3d_create(const ScrArea *area) {
	View3D *view3d = MEM_callocN(sizeof(View3D), "SpaceLink::View3D");

	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "View3D::Main");
		LIB_addtail(&view3d->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;
	}

	return (SpaceLink *)view3d;
}

ROSE_INLINE void view3d_free(SpaceLink *link) {
}

ROSE_INLINE void view3d_init(WindowManager *wm, ScrArea *area) {
}

ROSE_INLINE void view3d_exit(WindowManager *wm, ScrArea *area) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name View3D Main Region Methods
 * \{ */

void view3d_main_region_draw(struct rContext *C, ARegion *region) {
	GPU_clear_color(0.75f, 0.75f, 0.7f, 1.0f);

	uiBlock *block;
	if((block = UI_block_begin(C, region, "block-one"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 2, region->sizey - 2, 0, 0);
		do {
			uiLayout *global = UI_layout_col(root, 2);
			do {
				uiLayout *layout = UI_layout_row(global, 2);
				uiDefBut(block, UI_BTYPE_TXT, "one", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y);
				uiDefBut(block, UI_BTYPE_TXT, "two", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y);
				uiDefBut(block, UI_BTYPE_TXT, "three", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y);
			} while (false);
			do {
				uiLayout *layout = UI_layout_row(global, 2);
				uiDefBut(block, UI_BTYPE_TXT, "one", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y);
				uiDefBut(block, UI_BTYPE_TXT, "two", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y);
			} while (false);
		} while (false);
		block->bounds = 2;
		UI_block_end(C, block);
	}
}

void view3d_main_region_init(ARegion *region) {
}
void view3d_main_region_exit(ARegion *region) {
}

/** \} */

void ED_spacetype_view3d() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::View3D");

	st->spaceid = SPACE_VIEW3D;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "View3D");

	st->create = view3d_create;
	st->free = view3d_free;
	st->init = view3d_init;
	st->exit = view3d_exit;

	// Main Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "View3D::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->draw = view3d_main_region_draw;
		art->init = view3d_main_region_init;
		art->exit = view3d_main_region_exit;
	}

	KER_spacetype_register(st);
}
