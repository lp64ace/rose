#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"
#include "DNA_userdef_types.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "UI_interface.h"
#include "UI_resource.h"

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
/** \name UserPref Header Region Methods
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name UserPref Main Region Methods
 * \{ */

ROSE_STATIC void userpref_main_region_theme_but(struct rContext *C, uiBut *but, Theme *theme, char direction) {
	WindowManager *wm = CTX_wm_manager(C);

	if (LIB_haslink(&U.themes, theme)) {
		switch (direction) {
			case 'D': {
				Theme *prev = theme->prev;
				LIB_remlink(&U.themes, theme);
				LIB_insertlinkbefore(&U.themes, prev, theme);
			} break;
			case 'U': {
				Theme *next = theme->next;
				LIB_remlink(&U.themes, theme);
				LIB_insertlinkafter(&U.themes, next, theme);
			} break;
		}
	}

	LISTBASE_FOREACH(wmWindow *, window, &wm->windows) {
		ED_screen_refresh(wm, window);
	}
}

ROSE_STATIC void userpref_main_region_layout_theme_widget_color(ARegion *region, uiBlock *block, uiLayout *root, const char *name, uiWidgetColors *wcol) {
	uiBut *but;

	uiLayout *col = UI_layout_col(root, PIXELSIZE);

	but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0.25 * UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	but = uiDefBut(block, UI_BTYPE_TEXT, name, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	but = uiDefBut(block, UI_BTYPE_HSPR, "", 0, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
	but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0.25 * UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);

	do {
		uiLayout *grid = UI_layout_grid(col, 5, true,  false);

		uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Param", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);
		uiButEnableFlag(uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
		uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Value", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);
		uiButEnableFlag(uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
		uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Descr", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);

		but = uiDefBut(block, UI_BTYPE_TEXT, "Outline", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_EDIT, "", 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->outline, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
		UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
		UI_but_func_set(but, userpref_main_region_theme_but, NULL, '-');
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_TEXT, "Default", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

		but = uiDefBut(block, UI_BTYPE_TEXT, "Inner1", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_EDIT, "", 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->inner, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
		UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
		UI_but_func_set(but, userpref_main_region_theme_but, NULL, '-');
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_TEXT, "Default", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

		but = uiDefBut(block, UI_BTYPE_TEXT, "Inner2", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_EDIT, "", 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->inner_sel, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
		UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
		UI_but_func_set(but, userpref_main_region_theme_but, NULL, '-');
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_TEXT, "Selected", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

		but = uiDefBut(block, UI_BTYPE_TEXT, "Text1", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_EDIT, "", 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->text, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
		UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
		UI_but_func_set(but, userpref_main_region_theme_but, NULL, '-');
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_TEXT, "Default", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

		but = uiDefBut(block, UI_BTYPE_TEXT, "Text2", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_EDIT, "", 4 * UI_UNIT_X, UI_UNIT_Y, &wcol->text_sel, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
		UI_but_func_text_set(but, uiButHandleTextFunc_Integer, 0x00000000u, 0xffffffffu);
		UI_but_func_set(but, userpref_main_region_theme_but, NULL, '-');
		but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_TEXT, "Selected", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

		UI_block_layout_set_current(block, root);
	} while (false);
}

void userpref_main_region_layout(struct rContext *C, ARegion *region) {
	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "USERPREF_theme_order"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, 0, region->sizey, 0, PIXELSIZE);

		but = uiDefBut(block, UI_BTYPE_TEXT, "[Themes]", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_HSPR, "", region->sizex, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_SEPR, "", region->sizex, 0.25 * UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);

		do {
			uiLayout *layout = UI_layout_grid(root, 5, true, false);

			int dynamic = ROSE_MAX(PIXELSIZE, region->sizex - (2 * 1 * UI_UNIT_X + 2 * PIXELSIZE));

			uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Name", dynamic, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);
			uiButEnableFlag(uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
			uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "", 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);
			uiButEnableFlag(uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
			uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "", 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);

			LISTBASE_FOREACH(Theme *, theme, &U.themes) {
				but = uiDefBut(block, UI_BTYPE_TEXT, theme->name, dynamic, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT);
				but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
				but = uiDefBut(block, UI_BTYPE_PUSH, "+", 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_CENTER);
				if (theme != U.themes.first) {
					UI_but_func_set(but, userpref_main_region_theme_but, theme, 'U');
				}
				but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
				but = uiDefBut(block, UI_BTYPE_PUSH, "-", 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_CENTER);
				if (theme != U.themes.last) {
					UI_but_func_set(but, userpref_main_region_theme_but, theme, 'D');
				}
			}

			UI_block_layout_set_current(block, root);
		} while (false);

		but = uiDefBut(block, UI_BTYPE_SEPR, "", region->sizex, 0.25 * UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_TEXT, "[Global]", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_HSPR, "", region->sizex, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_SEPR, "", region->sizex, 0.25 * UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);

		do {
			uiLayout *layout = UI_layout_grid(root, 5, true, false);

			uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Param", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);
			uiButEnableFlag(uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
			uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Value", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);
			uiButEnableFlag(uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0), UI_DISABLED);
			uiButEnableFlag(uiDefBut(block, UI_BTYPE_TEXT, "Descr", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 64, UI_BUT_TEXT_LEFT), UI_DISABLED);

			Theme *theme = UI_GetTheme();

			but = uiDefBut(block, UI_BTYPE_TEXT, "Caret", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
			but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			but = uiDefBut(block, UI_BTYPE_EDIT, "Value", 4 * UI_UNIT_X, UI_UNIT_Y, &theme->tui.text_cur, UI_POINTER_UINT, 0, UI_BUT_TEXT_LEFT | UI_BUT_HEX);
			UI_but_func_set(but, userpref_main_region_theme_but, theme, '-');
			but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			but = uiDefBut(block, UI_BTYPE_TEXT, "Default", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);

			UI_block_layout_set_current(block, root);
		} while (false);

		but = uiDefBut(block, UI_BTYPE_SEPR, "", region->sizex, 0.25 * UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_TEXT, "[Widgets]", 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_HSPR, "", region->sizex, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
		but = uiDefBut(block, UI_BTYPE_SEPR, "", region->sizex, 0.25 * UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);

		do {

			uiLayout *layout = UI_layout_grid(root, 3, true, false);

			Theme *theme = UI_GetTheme();

			userpref_main_region_layout_theme_widget_color(region, block, layout, "[Text]", &theme->tui.wcol_txt);
			but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, 0, NULL, UI_POINTER_NIL, 0, 0);
			userpref_main_region_layout_theme_widget_color(region, block, layout, "[Button]", &theme->tui.wcol_but);


			userpref_main_region_layout_theme_widget_color(region, block, layout, "[Edit]", &theme->tui.wcol_edit);
			but = uiDefBut(block, UI_BTYPE_VSPR, "", PIXELSIZE, 0, NULL, UI_POINTER_NIL, 0, 0);
			userpref_main_region_layout_theme_widget_color(region, block, layout, "[Separator]", &theme->tui.wcol_sepr);

			UI_block_layout_set_current(block, root);
		} while (false);

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
