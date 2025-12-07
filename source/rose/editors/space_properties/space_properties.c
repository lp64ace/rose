#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "GPU_framebuffer.h"
#include "GPU_texture.h"
#include "GPU_matrix.h"
#include "GPU_viewport.h"

#include "DRW_engine.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "UI_interface.h"
#include "UI_resource.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "WM_draw.h"

#include "KER_screen.h"

/* -------------------------------------------------------------------- */
/** \name View3D SpaceType Methods
 * \{ */

ROSE_INLINE SpaceLink *properties_create(const ScrArea *area) {
	SpacePropeties *properties = MEM_callocN(sizeof(SpacePropeties), "SpaceLink::SpacePropeties");

	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpacePropeties::Main");
		LIB_addtail(&properties->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;

		region->flag |= RGN_FLAG_ALWAYS_REDRAW;
	}
	properties->spacetype = SPACE_PROPERTIES;

	return (SpaceLink *)properties;
}

ROSE_INLINE void properties_free(SpaceLink *link) {
}

ROSE_INLINE void properties_init(WindowManager *wm, ScrArea *area) {
}

ROSE_INLINE void properties_exit(WindowManager *wm, ScrArea *area) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Properties Main Region Methods
 * \{ */

ROSE_INLINE void propeties_main_region_layout(struct rContext *C, ARegion *region) {
	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "PROPERTIES_main"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 0, region->sizey, 0, 0);
		uiLayout *layout = UI_layout_col(root, PIXELSIZE);
		but = uiDefBut(block, UI_BTYPE_TEXT, "Read Me", 0, 0, 8 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, 0);
		but = uiDefBut(block, UI_BTYPE_EDIT, "Edit Me", 0, 0, 8 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, 0);
		UI_block_end(C, block);
	}
}

/** \} */

void ED_spacetype_properties() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::SpacePropeties");

	st->spaceid = SPACE_PROPERTIES;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "SpacePropeties");

	st->create = properties_create;
	st->free = properties_free;
	st->init = properties_init;
	st->exit = properties_exit;

	// Main Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpacePropeties::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->layout = propeties_main_region_layout;
		art->draw = NULL;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}

	KER_spacetype_register(st);
}
