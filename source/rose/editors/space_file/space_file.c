#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "UI_interface.h"
#include "UI_view2d.h"

#include "RNA_prototypes.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"
#include "LIB_path_utils.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_main.h"
#include "KER_screen.h"

#include "WM_api.h"

#include "file.h"
#include "filelist/filelist.h"

#include <stdio.h>

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
		region->alignment = RGN_ALIGN_LEFT;
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
	SpaceFile *sfile = (SpaceFile *)link;

	if (sfile->files) {
		filelist_free(sfile->files);
		sfile->files = NULL;
	}
}

ROSE_INLINE void file_init(WindowManager *wm, ScrArea *area) {
	SpaceFile *sfile = (SpaceFile *)area->spacedata.first;
	FileSelectParams *params = ED_fileselect_ensure_active_params(sfile);

	if (!sfile->files) {
		sfile->files = filelist_new(params->type);
	}

	filelist_setdir(sfile->files, params->dir);
	filelist_settype(sfile->files, params->type);
}

ROSE_INLINE void file_refresh(const rContext *C, ScrArea *area) {
	SpaceFile *sfile = CTX_wm_space_file(C);
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);

	filelist_setdir(sfile->files, params->dir);
	filelist_settype(sfile->files, params->type);

	if (filelist_needs_reading(sfile->files)) {
		if (!filelist_pending(sfile->files)) {
			filelist_readjob_start(sfile->files, C);
		}
	}
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
		UI_layout_scale_x_set(row, 0.8f);

		wmOperatorType *ot = WM_operatortype_find("FILE_OT_cancel", false);

		uiBut *but = uiDefBut(block, UI_BTYPE_PUSH, "Cancel", 0, 0, 5 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, 0);
		UI_but_op_set(but, ot);
	}
}

static void file_panel_execution_execute_button(uiLayout *layout, const char *name) {
	uiLayout *row = UI_layout_row(layout, false);

	uiBlock *block;
	if ((block = UI_layout_block(row))) {
		UI_layout_scale_x_set(row, 0.8f);

		wmOperatorType *ot = WM_operatortype_find("FILE_OT_execute", false);

		uiBut *but = uiDefBut(block, UI_BTYPE_PUSH, name, 0, 0, 5 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, 0);
		UI_but_op_set(but, ot);
	}
}

ROSE_INLINE void file_panel_execution_buttons_draw(const rContext *C, Panel *panel) {
	Screen *screen = CTX_wm_screen(C);
	ScrArea *area = CTX_wm_area(C);
	ARegion *region = CTX_wm_region(C);

	SpaceFile *file = CTX_wm_space_file(C);
	FileSelectParams *params = ED_fileselect_get_active_params(file);
	PointerRNA ptr = RNA_pointer_create_discrete(&screen->id, &RNA_FileSelectParams, params);

	uiBut *but;
	uiBlock *block;
	if ((block = UI_layout_block(panel->layout))) {
		uiLayout *flow = UI_layout_grid(panel->layout, 0, false, false);

		uiLayout *subrow;
		if ((subrow = UI_layout_row(flow, BORDERPADDING))) {
			uiLayout *subsubrow;

			but = uiDefBut_RNA(block, UI_BTYPE_EDIT, "", 0, 0, 0, UI_UNIT_Y, &ptr, "filename", -1, UI_BUT_TEXT_LEFT);

			if ((subsubrow = UI_layout_row(subrow, BORDERPADDING))) {
				file_panel_execution_execute_button(subsubrow, params->title);
				file_panel_execution_cancel_button(subsubrow);
			}
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
/** \name File UI Region Methods
 * \{ */

ROSE_INLINE void file_ui_region_init(WindowManager *wm, ARegion *region) {
	ED_region_panels_init(wm, region);
}

ROSE_INLINE void file_panel_ui_file_select_path_draw(const rContext *C, Panel *panel) {
	Screen *screen = CTX_wm_screen(C);
	ScrArea *area = CTX_wm_area(C);
	ARegion *region = CTX_wm_region(C);

	SpaceFile *sfile = CTX_wm_space_file(C);
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	PointerRNA ptr = RNA_pointer_create_discrete(&screen->id, &RNA_FileSelectParams, params);

	uiBut *but;
	uiBlock *block;
	if ((block = UI_layout_block(panel->layout))) {
		uiLayout *flow = UI_layout_grid(panel->layout, 0, false, false);

		uiLayout *subrow;
		if ((subrow = UI_layout_row(flow, BORDERPADDING))) {
			uiLayout *subsubrow;

			if ((subsubrow = UI_layout_row(subrow, BORDERPADDING))) {
				uiDefBut(block, UI_BTYPE_PUSH, "\u2190", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, 0);  // Back
				uiDefBut(block, UI_BTYPE_PUSH, "\u2192", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, 0);  // Next
				uiDefBut(block, UI_BTYPE_PUSH, "\u2191", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0, 0);  // Parent
			}

			UI_block_layout_set_current(block, subrow);

			but = uiDefBut_RNA(block, UI_BTYPE_EDIT, "", 0, 0, 0, UI_UNIT_Y, &ptr, "directory", -1, UI_BUT_TEXT_LEFT);
			UI_but_func_set(but, file_directory_enter_handle, NULL, NULL);
		}
	}
}

void file_ui_region_panels_register(ARegionType *art) {
	PanelType *pt = MEM_callocN(sizeof(PanelType), "SpaceType::File::PanelType::UI");
	LIB_strcpy(pt->idname, ARRAY_SIZE(pt->idname), "FILEBROWSER_PT_directory_path");
	LIB_strcpy(pt->label, ARRAY_SIZE(pt->idname), "Directory Path");
	pt->flag = PANEL_TYPE_NO_HEADER;
	pt->draw = file_panel_ui_file_select_path_draw;
	LIB_addtail(&art->paneltypes, pt);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Main Region Methods
 * \{ */

ROSE_INLINE file_main_region_init(WindowManager *wm, ARegion *region) {
	wmKeyMap *keymap;

	if ((keymap = WM_keymap_ensure(wm->runtime.defaultconf, "File Browser", SPACE_FILE, RGN_TYPE_WINDOW)) != NULL) {
		WM_event_add_keymap_handler(&region->handlers, keymap);
	}

	UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_LIST, region->sizex, region->sizey);
}

bool file_main_region_needs_refresh_before_draw(SpaceFile *sfile) {
	/* Needed, because filelist is not initialized on loading */
	if (!sfile->files || filelist_needs_reading(sfile->files)) {
		return true;
	}

	return false;
}

void file_main_region_file_list_draw(rContext *C, ARegion *region) {
	Screen *screen = CTX_wm_screen(C);
	ScrArea *area = CTX_wm_area(C);

	View2D *v2d = &region->v2d;

	SpaceFile *sfile = CTX_wm_space_file(C);
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	FileList *files = sfile->files;

	if (file_main_region_needs_refresh_before_draw(sfile)) {
		file_refresh(C, NULL);
	}

	/* v2d has initialized flag, so this call will only set the mask correct */
	UI_view2d_region_reinit(v2d, V2D_COMMONVIEW_LIST, region->sizex, region->sizey);

	size_t numfiles = filelist_files_ensure(files);

	UI_view2d_view_ortho(v2d);

	uiBlock *block;
	if ((block = UI_block_begin(C, region, "FILEBROWSER_PT_file_list"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_VERTICAL, ITEM_LAYOUT_ROOT, region->v2d.cur.xmin, -region->v2d.cur.ymax, region->sizex, 0);
		uiLayout *col = UI_layout_col(root, 0);

		for (size_t index = 0; index < numfiles; index++) {
			FileDirEntry *file = filelist_file(files, index);

			do {
				uiLayout *row = UI_layout_row(col, 0);

				if (file->size && (file->draw_data.size[0] == '\0')) {
					LIB_strnformat_byte_size(file->draw_data.size, ARRAY_SIZE(file->draw_data.size), file->size, 1);
				}

				void *ptr = POINTER_FROM_INT(index);

				uiBut *but;
				but = uiDefBut(block, UI_BTYPE_TEXT, file->name, 0, 0, 0, UI_UNIT_Y, ptr, UI_POINTER_NIL, 0, 0, UI_BUT_TEXT_LEFT);
				UI_but_row_set(but, index & 1);

				if ((file->flag & FILE_SEL_SELECTED) != 0) {
					but->flag |= UI_SELECT;
				}

				if ((file->flag & FILE_SEL_HIGHLIGHTED) != 0) {
					but->flag |= UI_HOVER;
				}

				but = uiDefBut(block, UI_BTYPE_TEXT, file->draw_data.size, 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, ptr, UI_POINTER_NIL, 0, 0, UI_BUT_TEXT_LEFT);
				UI_but_row_set(but, index & 1);
				but = uiDefBut(block, UI_BTYPE_TEXT, file->draw_data.date, 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, ptr, UI_POINTER_NIL, 0, 0, UI_BUT_TEXT_LEFT);
				UI_but_row_set(but, index & 1);
			} while (false);
		}

		UI_block_end(C, block);

		UI_view2d_tot_rect_set(v2d, LIB_rctf_size_x(&block->rect), LIB_rctf_size_y(&block->rect));
	}

	ED_region_pixelspace(region);

	UI_view2d_scrollers_draw(v2d, &v2d->mask);
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
	st->keymap = file_keymap;
	st->keymapflag = ED_KEYMAP_UI;
	st->operatortypes = file_operatortypes;

	// Main
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "File::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->draw = file_main_region_file_list_draw;
		art->init = file_main_region_init;
		art->exit = ED_region_default_exit;
		art->keymapflag = ED_KEYMAP_UI | ED_KEYMAP_VIEW2D;
	}
	// Header
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "File::ARegionType::Header");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_HEADER;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
		art->keymapflag = ED_KEYMAP_UI;
	}
	// Execute
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "File::ARegionType::Execute");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_EXECUTE;
		art->draw = ED_region_default_draw;
		art->init = file_execution_region_init;
		art->exit = ED_region_default_exit;
		art->keymapflag = ED_KEYMAP_UI;
		file_execute_region_panels_register(art);
	}
	// UI
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "File::ARegionType::UI");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_UI;
		art->draw = ED_region_default_draw;
		art->init = file_ui_region_init;
		art->exit = ED_region_default_exit;
		art->keymapflag = ED_KEYMAP_UI;
		file_ui_region_panels_register(art);
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
		art->keymapflag = ED_KEYMAP_UI;
	}

	KER_spacetype_register(st);
}
