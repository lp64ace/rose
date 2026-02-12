#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "UI_interface.h"

#include "RNA_prototypes.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "WM_api.h"

#include "file.h"

/* -------------------------------------------------------------------- */
/** \name SpaceFile SpaceType Methods
 * \{ */

ROSE_INLINE SpaceLink *file_create(const ScrArea *area) {
	SpaceFile *file = MEM_callocN(sizeof(SpaceFile), "SpaceLink::SpaceFile");

	// Header
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceFile::Header");
		LIB_addtail(&file->regionbase, region);
		region->regiontype = RGN_TYPE_HEADER;
		region->alignment = RGN_ALIGN_TOP;
	}
	// Tools Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceFile::Tools");
		LIB_addtail(&file->regionbase, region);
		region->regiontype = RGN_TYPE_TOOLS;
		region->alignment = RGN_ALIGN_TOP;
	}
	// UI List Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceFile::UI");
		LIB_addtail(&file->regionbase, region);
		region->regiontype = RGN_TYPE_UI;
		region->alignment = RGN_ALIGN_TOP;
		region->flag = RGN_FLAG_DYNAMIC_SIZE | RGN_FLAG_NO_USER_RESIZE;
	}
	// Execute Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceFile::Execute");
		LIB_addtail(&file->regionbase, region);
		region->regiontype = RGN_TYPE_EXECUTE;
		region->alignment = RGN_ALIGN_BOTTOM;
		region->flag = RGN_FLAG_DYNAMIC_SIZE | RGN_FLAG_NO_USER_RESIZE;
	}
	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "SpaceFile::Main");
		LIB_addtail(&file->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;
		region->v2d.scroll = (V2D_SCROLL_RIGHT | V2D_SCROLL_BOTTOM);
		region->v2d.align = (V2D_ALIGN_NO_NEG_X | V2D_ALIGN_NO_POS_Y);
		region->v2d.keepzoom = (V2D_LOCKZOOM_X | V2D_LOCKZOOM_Y | V2D_LIMITZOOM | V2D_KEEPASPECT);
		region->v2d.keeptot = V2D_KEEPTOT_STRICT;
		region->v2d.minzoom = region->v2d.maxzoom = 1.0f;
	}
	file->spacetype = SPACE_FILE;

	return (SpaceLink *)file;
}

ROSE_INLINE void file_free(SpaceLink *link) {
}

ROSE_INLINE void file_init(WindowManager *wm, ScrArea *area) {
	SpaceFile *file = (SpaceFile *)area->spacedata.first;
	
	FileSelectParams *params = ED_fileselect_ensure_active_params(file);
}

ROSE_INLINE void file_exit(WindowManager *wm, ScrArea *area) {
	SpaceFile *file = (SpaceFile *)area->spacedata.first;

	MEM_SAFE_FREE(file->params);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Header Region Methods
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Execute Region Methods
 * \{ */

ROSE_INLINE void file_execution_region_init(WindowManager *wm, ARegion *region) {
	wmKeyMap *keymap;

	ED_region_panels_init(wm, region);
	region->v2d.keepzoom |= V2D_LOCKZOOM_X | V2D_LOCKZOOM_Y;

	/* own keymap */
	keymap = WM_keymap_ensure(wm->runtime.defaultconf, "File Browser", SPACE_FILE, RGN_TYPE_WINDOW);
	// WM_event_add_keymap_handler_v2d_mask(&region->handlers, keymap);
}

static void file_panel_execution_cancel_button(uiLayout *layout) {
	uiLayout *row = UI_layout_row(layout, false);

	uiBlock *block;
	if ((block = UI_layout_block(row))) {
		uiBut *but = uiDefBut(block, UI_BTYPE_PUSH, "Cancel", 0, 0, 5 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, UI_BUT_TEXT_LEFT);
	}
}

static void file_panel_execution_execute_button(uiLayout *layout, const char *name) {
	uiLayout *row = UI_layout_row(layout, false);

	uiBlock *block;
	if ((block = UI_layout_block(row))) {

		uiBut *but;
		if (name[0]) {
			but = uiDefBut(block, UI_BTYPE_PUSH, name, 0, 0, 5 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, UI_BUT_TEXT_LEFT);
		}
		else {
			but = uiDefBut(block, UI_BTYPE_PUSH, "Done", 0, 0, 5 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, UI_BUT_TEXT_LEFT);
		}
	}
}

ROSE_INLINE void file_panel_execution_buttons_draw(const rContext *C, Panel *panel) {
	Screen *screen = CTX_wm_screen(C);
	ScrArea *area = CTX_wm_area(C);
	ARegion *region = CTX_wm_region(C);

	SpaceFile *file = (SpaceFile *)area->spacedata.first;
	FileSelectParams *params = ED_fileselect_get_active_params(file);
	PointerRNA ptr = RNA_pointer_create_discrete(&screen->id, &RNA_FileSelectParams, params);

	uiBut *but;
	uiBlock *block;
	if ((block = UI_layout_block(panel->layout))) {
		uiLayout *layout = UI_layout_row(panel->layout, BORDERPADDING);
		UI_layout_scale_y_set(layout, 1.3f);

		but = uiDefBut_RNA(block, UI_BTYPE_EDIT, "Filename", 0, 0, 5 * UI_UNIT_X, UI_UNIT_Y, &ptr, "filename", -1, UI_BUT_TEXT_LEFT);

		{
			uiLayout *sub = UI_layout_row(layout, BORDERPADDING);

			file_panel_execution_execute_button(sub, params->title);
			file_panel_execution_cancel_button(sub);
		}
	}
}

void file_execute_region_panels_register(ARegionType *art) {
	PanelType *pt = MEM_callocN(sizeof(PanelType), "SpaceType::File::PanelType::Execute");
	LIB_strcpy(pt->idname, ARRAY_SIZE(pt->idname), "FILE_PT_execution_buttons");
	LIB_strcpy(pt->label, ARRAY_SIZE(pt->label), "Execute Buttons");
	pt->flag = PANEL_TYPE_NO_HEADER;
	pt->draw = file_panel_execution_buttons_draw;
	LIB_addtail(&art->paneltypes, pt);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Main Region Methods
 * \{ */

/** \} */

void ED_spacetype_file() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::File");

	st->spaceid = SPACE_FILE;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "File");

	st->create = file_create;
	st->free = file_free;
	st->init = file_init;
	st->exit = file_exit;

	// Main
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "File::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}
	// Header
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "File::ARegionType::Header");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_HEADER;
		art->prefsizey = UI_UNIT_Y;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}
	// Execute
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "File::ARegionType::Execute");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_EXECUTE;
		art->draw = ED_region_default_draw;
		art->init = file_execution_region_init;
		art->exit = ED_region_default_exit;
		file_execute_region_panels_register(art);
	}
	// UI
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "File::ARegionType::UI");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_UI;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}
	// Tools
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "File::ARegionType::Tools");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_TOOLS;
		art->prefsizex = 240;
		art->prefsizey = 60;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}

	KER_spacetype_register(st);
}
