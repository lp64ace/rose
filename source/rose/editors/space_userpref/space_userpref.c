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

#include <limits.h>
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
		region->vscroll = 1;
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
/** \name UserPref Header Region Methods
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name UserPref Main Region Methods
 * \{ */

enum {
	THEME_NONE,
	THEME_DEL,
	THEME_ADD,
	THEME_UP,
	THEME_DOWN,
};

void userpref_main_theme_update(struct rContext *C, uiBut *but, Theme *theme, int op) {
	if (ELEM(op, THEME_UP, THEME_DOWN) && (theme == NULL || !LIB_haslink(&U.themes, theme))) {
		return;
	}

	WindowManager *wm = CTX_wm_manager(C);

	switch (op) {
		case THEME_ADD: {
			theme = MEM_mallocN(sizeof(Theme), "Theme");
			memcpy(theme, U.themes.first, sizeof(Theme));
			LIB_addhead(&U.themes, theme);
		} break;
		case THEME_DEL: {
			if (!LIB_listbase_is_single(&U.themes)) {
				theme = U.themes.first;
				LIB_remlink(&U.themes, theme);
				MEM_freeN(theme);
			}
		} break;
		case THEME_UP: {
			LIB_remlink(&U.themes, theme);
			LIB_insertlinkbefore(&U.themes, theme->prev, theme);
		} break;
		case THEME_DOWN: {
			LIB_remlink(&U.themes, theme);
			LIB_insertlinkafter(&U.themes, theme->next, theme);
		} break;
	}

	LISTBASE_FOREACH(wmWindow *, window, &wm->windows) {
		ED_screen_refresh(wm, window);
	}
}

void userpref_main_region_layout(struct rContext *C, ARegion *region) {
	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "USERPREF_theme_order"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, region->sizex * region->hscroll, region->sizey * region->vscroll, 0, PIXELSIZE);

		but = uiDefBut(block, UI_BTYPE_TEXT, "Themes", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_HSPR, "", 0, 0, region->sizex, PIXELSIZE, NULL, 0, 0, 0);

		// Select Theme Grid

		do {
			uiLayout *grid = UI_layout_grid(root, 7, true, false);

			uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_VSPR, "(null)", 0, 0, PIXELSIZE, PIXELSIZE, NULL, 0, 0, UI_BUT_TEXT_LEFT), UI_DISABLED);
			uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Name", 0, 0, region->sizex - 3 * WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_LEFT), UI_DISABLED);
			but = uiDefBut(block, UI_BTYPE_VSPR, "(null)", 0, 0, PIXELSIZE, PIXELSIZE, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_PUSH, "x", 0, 0, WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
			UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_DEL);
			but = uiDefBut(block, UI_BTYPE_VSPR, "(null)", 0, 0, PIXELSIZE, PIXELSIZE, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_PUSH, "+", 0, 0, WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
			UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_ADD);
			but = uiDefBut(block, UI_BTYPE_SEPR, "(null)", 0, 0, V2D_SCROLL_WIDTH, PIXELSIZE, NULL, 0, 0, 0);

			LISTBASE_FOREACH(Theme *, theme, &U.themes) {
				but = uiDefBut(block, UI_BTYPE_VSPR, "(null)", 0, 0, PIXELSIZE, PIXELSIZE, NULL, 0, 0, 0);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, region->sizex - 3 * WIDGET_UNIT, WIDGET_UNIT, theme->name, UI_POINTER_STR, ARRAY_SIZE(theme->name), UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_VSPR, "(null)", 0, 0, PIXELSIZE, PIXELSIZE, NULL, 0, 0, 0);
				but = uiDefBut(block, UI_BTYPE_PUSH, theme->next ? "\xE2\x86\x93" : "", 0, 0, WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
				if (theme->next) {
					UI_but_func_set(but, userpref_main_theme_update, theme, THEME_DOWN);
				}
				but = uiDefBut(block, UI_BTYPE_VSPR, "(null)", 0, 0, PIXELSIZE, PIXELSIZE, NULL, 0, 0, 0);
				but = uiDefBut(block, UI_BTYPE_PUSH, theme->prev ? "\xE2\x86\x91" : "", 0, 0, WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
				if (theme->prev) {
					UI_but_func_set(but, userpref_main_theme_update, theme, THEME_UP);
				}
				but = uiDefBut(block, UI_BTYPE_SEPR, "(null)", 0, 0, V2D_SCROLL_WIDTH, PIXELSIZE, NULL, 0, 0, 0);
			}
			UI_block_layout_set_current(block, root);
		} while (false);

		but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

		Theme *theme = U.themes.first;

		do {
			uiLayout *row = UI_layout_row(root, 0);

			but = uiDefBut(block, UI_BTYPE_TEXT, "Theme: ", 0, 0, 4 * WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_LEFT);
			but = uiDefBut(block, UI_BTYPE_TEXT, (theme) ? theme->name : "(null)", 0, 0, region->sizex - 4 * WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_LEFT);
			
			UI_block_layout_set_current(block, root);

			if (!theme) {
				break;
			}

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_TEXT, "Widgets", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				uiWidgetColors *wcol = &theme->tui.wcol_txt;
				uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Text Widgets", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
				
				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Outline", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->outline, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Outline Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Inner1", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->inner, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Background Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Inner2", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->inner_sel, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Background Color (Hovered)", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Text", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->text, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Text Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Roundness", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->roundness, UI_POINTER_FLT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Decimal, 0.0, 0.1);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Corner Roundness 0.0 - 1.0", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				uiWidgetColors *wcol = &theme->tui.wcol_but;
				uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Push Widgets", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
				
				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Outline", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->outline, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Outline Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Inner1", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->inner, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Background Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Inner2", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->inner_sel, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Background Color (Pushed)", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Text", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->text, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Text Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Roundness", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->roundness, UI_POINTER_FLT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Decimal, 0.0, 0.1);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Corner Roundness 0.0 - 1.0", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				uiWidgetColors *wcol = &theme->tui.wcol_edit;
				uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Edit Widgets", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
				
				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Outline", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->outline, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Outline Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Inner1", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->inner, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Background Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Inner2", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->inner_sel, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Background Color (Active)", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Text1", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->text, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Text Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Text2", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->text_sel, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Text Color (Selected)", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				
				but = uiDefBut(block, UI_BTYPE_TEXT, "Roundness", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->roundness, UI_POINTER_FLT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Decimal, 0.0, 0.1);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Corner Roundness 0.0 - 1.0", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				uiWidgetColors *wcol = &theme->tui.wcol_sepr;
				uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Separator Widgets", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);

				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Line", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->text, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Separator Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Roundness", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->roundness, UI_POINTER_FLT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Decimal, 0.0, 0.1);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Corner Roundness 0.0 - 1.0", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_TEXT, "Misc", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				ThemeUI *tui = &theme->tui;

				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Caret", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &tui->text_cur, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Text Cursror Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_TEXT, "Spaces", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				ThemeSpace *ts = &theme->space_empty;
				uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Space::Empty", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);

				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header1", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header2", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header_hi, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color (Highlighted)", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Background", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->back, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Main Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				ThemeSpace *ts = &theme->space_view3d;
				uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Space::View3D", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);

				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header1", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header2", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header_hi, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color (Highlighted)", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Background", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->back, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Main Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				ThemeSpace *ts = &theme->space_topbar;
				uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Space::TopBar", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);

				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header1", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header2", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header_hi, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color (Highlighted)", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Background", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->back, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Main Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				ThemeSpace *ts = &theme->space_statusbar;
				uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Space::StatusBar", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);

				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header1", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header2", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header_hi, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color (Highlighted)", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Background", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->back, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Main Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, WIDGET_UNIT, NULL, 0, 0, 0);

			do {
				ThemeSpace *ts = &theme->space_userpref;
				uiButEnableFlag(but = uiDefBut(block, UI_BTYPE_TEXT, "Space::UserPref", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);

				uiLayout *grid = UI_layout_grid(root, 3, true, false);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header1", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Header2", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->header_hi, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Header Region Color (Highlighted)", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				but = uiDefBut(block, UI_BTYPE_TEXT, "Background", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_EDIT, "", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, &ts->back, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
				UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
				UI_but_func_set(but, userpref_main_theme_update, NULL, THEME_NONE);
				but = uiDefBut(block, UI_BTYPE_TEXT, "Main Region Color", 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

				UI_block_layout_set_current(block, root);
			} while (false);
		} while (false);

		UI_block_scroll(region, block, root);
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
		art->layout = NULL;
		art->draw = ED_region_header_draw;
		art->init = ED_region_header_init;
		art->exit = ED_region_header_exit;
	}
	// Main Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "SpaceUser::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->layout = userpref_main_region_layout;
		art->draw = ED_region_default_draw;
		art->init = ED_region_default_init;
		art->exit = ED_region_default_exit;
	}

	KER_spacetype_register(st);
}
