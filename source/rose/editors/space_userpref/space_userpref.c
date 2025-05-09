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

	memcpy(&user->editing, U.themes.first, sizeof(Theme));

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
	THEME_SEL,
	THEME_APPLY,
	THEME_RESET,
};

void userpref_main_theme_update(struct rContext *C, uiBut *but, void *vtheme, void *vop) {
	Theme *theme = (Theme *)vtheme;

	if (ELEM(POINTER_AS_INT(vop), THEME_SEL, THEME_DEL) && (theme == NULL || !LIB_haslink(&U.themes, theme))) {
		return;
	}

	WindowManager *wm = CTX_wm_manager(C);

	switch (POINTER_AS_INT(vop)) {
		case THEME_ADD: {
			theme = MEM_mallocN(sizeof(Theme), "Theme");
			memcpy(theme, U.themes.first, sizeof(Theme));
			LIB_strcat(theme->name, ARRAY_SIZE(theme->name), " (copy)");
			LIB_addhead(&U.themes, theme);
		} break;
		case THEME_DEL: {
			if (!LIB_listbase_is_single(&U.themes)) {
				LIB_remlink(&U.themes, theme);
				MEM_freeN(theme);
			}
		} break;
		case THEME_SEL: {
			LIB_remlink(&U.themes, theme);
			LIB_addhead(&U.themes, theme);
		} break;
		case THEME_APPLY: {
			memcpy(U.themes.first, theme, sizeof(Theme));
		} break;
		case THEME_RESET: {
			memcpy(theme, U.themes.first, sizeof(Theme));
		} break;
	}

	ScrArea *area = CTX_wm_area(C);
	SpaceUser *user = (SpaceUser *)area->spacedata.first;

	if (ELEM(POINTER_AS_INT(vop), THEME_APPLY, THEME_RESET)) {
		ScrArea *area = CTX_wm_area(C);

		ED_area_tag_redraw(area);
	}
	else {
		LISTBASE_FOREACH(wmWindow *, window, &wm->windows) {
			ED_screen_refresh(wm, window);
		}
	}

	memcpy(&user->editing, U.themes.first, sizeof(Theme));
}

void userpref_main_region_padding(struct uiBlock *block, int unit) {
	uiBut *but;
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 4 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
}

void userpref_main_region_theme_header(struct uiBlock *block, struct uiLayout *global, int unit) {
	uiBut *but;
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
	do {
		uiLayout *local = UI_layout_grid(global, 4, true, false);

		int flex = ROSE_MAX(WIDGET_UNIT, 2 * unit - 2 * WIDGET_UNIT);
		but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, flex, WIDGET_UNIT, NULL, 0, 0, 0);
		but = uiDefBut(block, UI_BTYPE_TEXT, "Themes", 0, 0, 2 * WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
		but = uiDefBut(block, UI_BTYPE_PUSH, "+", 0, 0, 1 * WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
		UI_but_func_set(but, userpref_main_theme_update, NULL, (void *)THEME_ADD);
		but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, flex, WIDGET_UNIT, NULL, 0, 0, 0);

		UI_block_layout_set_current(block, global);
	} while(false);
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
}

void userpref_main_region_theme_item(struct uiBlock *block, struct uiLayout *global, struct Theme *theme, int unit) {
	uiBut *but;
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
	do {
		uiLayout *local = UI_layout_grid(global, 4, true, false);

		int flex = ROSE_MAX(WIDGET_UNIT, 4 * unit - 3 * WIDGET_UNIT);
		but = uiDefBut(block, UI_BTYPE_EDIT, "(placeholder)", 0, 0, flex, WIDGET_UNIT, &theme->name, UI_POINTER_STR, ARRAY_SIZE(theme->name), UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_PUSH, "\u2713", 0, 0, WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
		UI_but_func_set(but, userpref_main_theme_update, theme, (void *)THEME_SEL);
		but = uiDefBut(block, UI_BTYPE_PUSH, "\u2716", 0, 0, WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
		UI_but_func_set(but, userpref_main_theme_update, theme, (void *)THEME_DEL);
		if (LIB_listbase_is_single(&U.themes)) {
			uiButEnableFlag(but, UI_DISABLED);
		}

		UI_block_layout_set_current(block, global);
	} while(false);
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
}

void userpref_main_region_widget_header(struct uiBlock *block, struct uiLayout *global, const char *nwidget, int unit) {
	uiBut *but;
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
	but = uiDefBut(block, UI_BTYPE_TEXT, nwidget, 0, 0, 4 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
}

void userpref_main_region_widget_item_color(struct uiBlock *block, struct uiLayout *global, const char *iname, const char *idscr, unsigned char ptr[4], int unit) {
	uiBut *but;
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
	do {
		uiLayout *local = UI_layout_grid(global, 3, true, false);
		int flex = ROSE_MAX(WIDGET_UNIT, 2 * unit - 2 * WIDGET_UNIT);
		but = uiDefBut(block, UI_BTYPE_TEXT, iname, 0, 0, flex, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_EDIT, "(nil)", 0, 0, 4 * WIDGET_UNIT, WIDGET_UNIT, ptr, UI_POINTER_UINT, 0, UI_BUT_HEX);
		but = uiDefBut(block, UI_BTYPE_TEXT, idscr, 0, 0, flex, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_RIGHT);
		UI_block_layout_set_current(block, global);
	} while(false);
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
}

void userpref_main_region_widget_item_float(struct uiBlock *block, struct uiLayout *global, const char *iname, const char *idscr, float *ptr, int unit) {
	uiBut *but;
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
	do {
		uiLayout *local = UI_layout_grid(global, 3, true, false);
		int flex = ROSE_MAX(WIDGET_UNIT, 2 * unit - 2 * WIDGET_UNIT);
		but = uiDefBut(block, UI_BTYPE_TEXT, iname, 0, 0, flex, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_EDIT, "(nil)", 0, 0, 4 * WIDGET_UNIT, WIDGET_UNIT, ptr, UI_POINTER_FLT, 0, UI_BUT_HEX);
		but = uiDefBut(block, UI_BTYPE_TEXT, idscr, 0, 0, flex, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_RIGHT);
		UI_block_layout_set_current(block, global);
	} while(false);
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
}

void userpref_main_region_theme_buttons(struct uiBlock *block, struct uiLayout *global, struct Theme *editing, int unit) {
	uiBut *but;
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
	do {
		uiLayout *local = UI_layout_grid(global, 5, true, false);
		int flex = ROSE_MAX(WIDGET_UNIT, 2 * unit - 2.5 * WIDGET_UNIT);
		but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, flex, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_PUSH, "Apply", 0, 0, 2 * WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
		UI_but_func_set(but, userpref_main_theme_update, editing, (void *)THEME_APPLY);
		but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
		but = uiDefBut(block, UI_BTYPE_PUSH, "Reset", 0, 0, 2 * WIDGET_UNIT, WIDGET_UNIT, NULL, 0, 0, 0);
		UI_but_func_set(but, userpref_main_theme_update, editing, (void *)THEME_RESET);
		but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, flex, WIDGET_UNIT, NULL, 0, 0, UI_BUT_TEXT_RIGHT);
		UI_block_layout_set_current(block, global);
	} while(false);
	but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
}

void userpref_main_region_layout(struct rContext *C, ARegion *region) {
	ScrArea *area = CTX_wm_area(C);
	SpaceUser *user = (SpaceUser *)area->spacedata.first;

	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "USERPREF_theme_order"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, region->sizex * region->hscroll, region->sizey * region->vscroll, 0, 0);

		int unit = ROSE_MAX(WIDGET_UNIT, region->sizex / 6);

		do {
			uiLayout *global = UI_layout_grid(root, 3, true, false);

			but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 4 * unit, WIDGET_UNIT, NULL, 0, 0, 0);
			but = uiDefBut(block, UI_BTYPE_SEPR, "(pad)", 0, 0, 1 * unit, WIDGET_UNIT, NULL, 0, 0, 0);

			userpref_main_region_theme_header(block, global, unit);
			userpref_main_region_padding(block, unit);
			LISTBASE_FOREACH(Theme *, theme, &U.themes) {
				userpref_main_region_theme_item(block, global, theme, unit);
			}
			userpref_main_region_padding(block, unit);
			userpref_main_region_widget_header(block, global, "Text", unit);
			userpref_main_region_padding(block, unit);
			do {
				uiWidgetColors *wc = &(&user->editing)->tui.wcol_txt;
				userpref_main_region_widget_item_color(block, global, "Inner1", "Default background color", wc->inner, unit);
				userpref_main_region_widget_item_color(block, global, "Inner2", "Hovered background color", wc->inner_sel, unit);
				userpref_main_region_widget_item_color(block, global, "Text", "Text foreground color", wc->text, unit);
			} while(false);
			userpref_main_region_padding(block, unit);
			userpref_main_region_widget_header(block, global, "Push", unit);
			userpref_main_region_padding(block, unit);
			do {
				uiWidgetColors *wc = &(&user->editing)->tui.wcol_but;
				userpref_main_region_widget_item_color(block, global, "Inner1", "Default background color", wc->inner, unit);
				userpref_main_region_widget_item_color(block, global, "Inner2", "Hovered background color", wc->inner_sel, unit);
				userpref_main_region_widget_item_color(block, global, "Text", "Text foreground color", wc->text, unit);
			} while(false);
			userpref_main_region_padding(block, unit);
			userpref_main_region_widget_header(block, global, "Edit", unit);
			userpref_main_region_padding(block, unit);
			do {
				uiWidgetColors *wc = &(&user->editing)->tui.wcol_but;
				userpref_main_region_widget_item_color(block, global, "Inner1", "Default background color", wc->inner, unit);
				userpref_main_region_widget_item_color(block, global, "Inner2", "Hovered background color", wc->inner_sel, unit);
				userpref_main_region_widget_item_color(block, global, "Text1", "Default text foreground color", wc->text, unit);
				userpref_main_region_widget_item_color(block, global, "Text2", "Hovered text foreground color", wc->text_sel, unit);
			} while(false);
			userpref_main_region_padding(block, unit);
			userpref_main_region_widget_header(block, global, "Separator", unit);
			userpref_main_region_padding(block, unit);
			do {
				uiWidgetColors *wc = &(&user->editing)->tui.wcol_sepr;
				userpref_main_region_widget_item_color(block, global, "Line", "Default line foreground color", wc->text, unit);
			} while(false);
			userpref_main_region_padding(block, unit);
			userpref_main_region_widget_header(block, global, "Scrollbar", unit);
			userpref_main_region_padding(block, unit);
			do {
				uiWidgetColors *wc = &(&user->editing)->tui.wcol_scroll;
				userpref_main_region_widget_item_color(block, global, "Inner1", "Default background color", wc->inner, unit);
				userpref_main_region_widget_item_color(block, global, "Inner2", "Hovered background color", wc->inner_sel, unit);
				userpref_main_region_widget_item_color(block, global, "Thumb1", "Default thumb color", wc->text, unit);
				userpref_main_region_widget_item_color(block, global, "Thumb2", "Hovered thumb color", wc->text_sel, unit);
			} while(false);
			userpref_main_region_padding(block, unit);
			userpref_main_region_theme_buttons(block, global, &user->editing, unit);
			userpref_main_region_padding(block, unit);
		} while(false);

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
