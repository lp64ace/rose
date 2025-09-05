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
#include "LIB_ghash.h"
#include "LIB_utildefines.h"

#include "RLO_readfile.h"
#include "RLO_writefile.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "WM_api.h"
#include "WM_window.h"

#include "RT_token.h"
#include "RT_parser.h"
#include "intern/ast/type.h"
#include "intern/genfile.h"

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

void userpref_main_theme_update(struct rContext *C, uiBut *but, void *vtheme, void *voperation) {
	Theme *theme = (Theme *)vtheme;

	if (ELEM(POINTER_AS_INT(voperation), THEME_SEL, THEME_DEL) && (theme == NULL || !LIB_haslink(&U.themes, theme))) {
		return;
	}

	WindowManager *wm = CTX_wm_manager(C);

	switch (POINTER_AS_INT(voperation)) {
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

	if (ELEM(POINTER_AS_INT(voperation), THEME_APPLY, THEME_RESET)) {
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

void userpref_main_region_layout_theme_list_edit(struct rContext *C, ARegion *region, uiBlock *block, uiLayout *root) {
	uiLayout *grid = UI_layout_grid(root, 3, true, false);
	LISTBASE_FOREACH(Theme *, theme, &U.themes) {
		uiBut *but;
		but = uiDefBut(block, UI_BTYPE_TEXT, "", 0, 0, 3 * UI_UNIT_X, UI_UNIT_Y, theme->name, UI_POINTER_STR, ARRAY_SIZE(theme->name), UI_BUT_TEXT_LEFT);
		but = uiDefBut(block, UI_BTYPE_PUSH, "\u2217", 0, 0, 4 * PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		UI_but_func_set(but, userpref_main_theme_update, theme, THEME_SEL);
		but = uiDefBut(block, UI_BTYPE_PUSH, "\u2A2F", 0, 0, 4 * PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		UI_but_func_set(but, userpref_main_theme_update, theme, THEME_DEL);
	}
	UI_block_layout_set_current(block, root);
}

void userpref_main_region_layout_theme_list(struct rContext *C, ARegion *region, uiBlock *block, uiLayout *root) {
	uiLayout *grid = UI_layout_grid(root, 3, true, false);
	uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	userpref_main_region_layout_theme_list_edit(C, region, block, grid);
	uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	UI_block_layout_set_current(block, root);
}

enum {
	UI_WIDGET_COLORS_ROUNDNESS = 1 << 0,
	UI_WIDGET_COLORS_INNERDEF = 1 << 1,
	UI_WIDGET_COLORS_INNERSEL = 1 << 2,
	UI_WIDGET_COLORS_TEXTDEF = 1 << 3,
	UI_WIDGET_COLORS_TEXTSEL = 1 << 4,
	UI_WIDGET_COLORS_THUMBDEF = 1 << 5,
	UI_WIDGET_COLORS_THUMBSEL = 1 << 6,

	UI_WIDGET_COLORS_SEPR = UI_WIDGET_COLORS_ROUNDNESS | UI_WIDGET_COLORS_INNERDEF,
	UI_WIDGET_COLORS_TEXT = UI_WIDGET_COLORS_ROUNDNESS | UI_WIDGET_COLORS_INNERDEF | UI_WIDGET_COLORS_INNERSEL | UI_WIDGET_COLORS_TEXTDEF,
	UI_WIDGET_COLORS_PUSH = UI_WIDGET_COLORS_ROUNDNESS | UI_WIDGET_COLORS_INNERDEF | UI_WIDGET_COLORS_INNERSEL | UI_WIDGET_COLORS_TEXTDEF,
	UI_WIDGET_COLORS_EDIT = UI_WIDGET_COLORS_ROUNDNESS | UI_WIDGET_COLORS_INNERDEF | UI_WIDGET_COLORS_INNERSEL | UI_WIDGET_COLORS_TEXTDEF | UI_WIDGET_COLORS_TEXTSEL,
	UI_WIDGET_COLORS_SCROLL = UI_WIDGET_COLORS_ROUNDNESS | UI_WIDGET_COLORS_INNERDEF | UI_WIDGET_COLORS_INNERSEL | UI_WIDGET_COLORS_THUMBDEF | UI_WIDGET_COLORS_THUMBSEL,
};

void userpref_main_region_layout_color(uiBlock *block, uiLayout *root, unsigned char *ptr) {
#if 1
	uiLayout *grid = UI_layout_grid(root, 5, true, false);
	uiDefBut(block, UI_BTYPE_TEXT, "RGBA", 0, 0, 0.4f * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_EDIT, "0x00", 0, 0, 0.4f * UI_UNIT_X, UI_UNIT_Y, ptr + 0, UI_POINTER_BYTE, 0, 0);
	uiDefBut(block, UI_BTYPE_EDIT, "0x00", 0, 0, 0.4f * UI_UNIT_X, UI_UNIT_Y, ptr + 1, UI_POINTER_BYTE, 0, 0);
	uiDefBut(block, UI_BTYPE_EDIT, "0x00", 0, 0, 0.4f * UI_UNIT_X, UI_UNIT_Y, ptr + 2, UI_POINTER_BYTE, 0, 0);
	uiDefBut(block, UI_BTYPE_EDIT, "0x00", 0, 0, 0.4f * UI_UNIT_X, UI_UNIT_Y, ptr + 3, UI_POINTER_BYTE, 0, 0);
	UI_block_layout_set_current(block, root);
#else
	uiDefBut(block, UI_BTYPE_EDIT, "Color", 0, 0, 1 * UI_UNIT_X, UI_UNIT_Y, ptr, UI_POINTER_UINT, 0, UI_BUT_HEX);
#endif
}

void userpref_main_region_layout_theme_edit_widget_post(uiBlock *block, uiLayout *root, uiWidgetColors *widget, int flag) {
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 4 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);

	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	do {
		uiLayout *grid = UI_layout_grid(root, 3, true, false);
		
		if (flag & UI_WIDGET_COLORS_ROUNDNESS) {
			uiDefBut(block, UI_BTYPE_TEXT, "Roundness", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
			uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			uiDefBut(block, UI_BTYPE_EDIT, "Roundness", 0, 0, 2 * UI_UNIT_X, UI_UNIT_Y, &widget->roundness, UI_POINTER_FLT, 0, UI_BUT_HEX);
		}

		if (flag & UI_WIDGET_COLORS_INNERDEF) {
			uiDefBut(block, UI_BTYPE_TEXT, "Default", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
			uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			userpref_main_region_layout_color(block, grid, widget->inner);
		}

		if (flag & UI_WIDGET_COLORS_INNERSEL) {
			uiDefBut(block, UI_BTYPE_TEXT, "Hovered", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
			uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			userpref_main_region_layout_color(block, grid, widget->inner_sel);
		}

		if (flag & UI_WIDGET_COLORS_TEXTDEF) {
			uiDefBut(block, UI_BTYPE_TEXT, "Text", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
			uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			userpref_main_region_layout_color(block, grid, widget->text);
		}

		if (flag & UI_WIDGET_COLORS_TEXTSEL) {
			uiDefBut(block, UI_BTYPE_TEXT, "Selection", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
			uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			userpref_main_region_layout_color(block, grid, widget->text_sel);
		}

		if (flag & UI_WIDGET_COLORS_THUMBDEF) {
			uiDefBut(block, UI_BTYPE_TEXT, "Thumb", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
			uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			userpref_main_region_layout_color(block, grid, widget->text);
		}

		if (flag & UI_WIDGET_COLORS_THUMBSEL) {
			uiDefBut(block, UI_BTYPE_TEXT, "Thumb Selected", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
			uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
			userpref_main_region_layout_color(block, grid, widget->text_sel);
		}
		
		UI_block_layout_set_current(block, root);
	} while (false);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);

	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 4 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
}

void userpref_main_region_layout_theme_edit_widget_pre(uiBlock *block, uiLayout *root, const char *name, uiWidgetColors *widget, int flag) {
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_TEXT, name, 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	userpref_main_region_layout_theme_edit_widget_post(block, root, widget, flag);
}

void userpref_main_region_layout_theme_edit_widget(struct rContext *C, ARegion *region, uiBlock *block, uiLayout *root) {
	ScrArea *area = CTX_wm_area(C);
	SpaceUser *user = (SpaceUser *)area->spacedata.first;

	uiLayout *grid = UI_layout_grid(root, 3, false, false);

	userpref_main_region_layout_theme_edit_widget_pre(block, grid, "Line", & user->editing.tui.wcol_sepr, UI_WIDGET_COLORS_SEPR);
	userpref_main_region_layout_theme_edit_widget_pre(block, grid, "Text", &user->editing.tui.wcol_txt, UI_WIDGET_COLORS_TEXT);
	userpref_main_region_layout_theme_edit_widget_pre(block, grid, "Push", &user->editing.tui.wcol_but, UI_WIDGET_COLORS_PUSH);
	userpref_main_region_layout_theme_edit_widget_pre(block, grid, "Edit", &user->editing.tui.wcol_edit, UI_WIDGET_COLORS_EDIT);
	userpref_main_region_layout_theme_edit_widget_pre(block, grid, "Scroll", &user->editing.tui.wcol_scroll, UI_WIDGET_COLORS_SCROLL);
	
	UI_block_layout_set_current(block, root);
}

void userpref_main_region_layout_theme_edit_spaces_post(uiBlock *block, uiLayout *root, ThemeSpace *space) {
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 4 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);

	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	do {
		uiLayout *grid = UI_layout_grid(root, 3, true, false);

		uiDefBut(block, UI_BTYPE_TEXT, "Header", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		userpref_main_region_layout_color(block, grid, space->header);

		uiDefBut(block, UI_BTYPE_TEXT, "Hovered", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		userpref_main_region_layout_color(block, grid, space->header_hi);

		uiDefBut(block, UI_BTYPE_TEXT, "Background", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, UI_BUT_TEXT_LEFT);
		uiDefBut(block, UI_BTYPE_VSPR, "Padding", 0, 0, PIXELSIZE, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		userpref_main_region_layout_color(block, grid, space->back);

		UI_block_layout_set_current(block, root);
	} while (false);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);

	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 4 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, PIXELSIZE, NULL, UI_POINTER_NIL, 0, 0);
}

void userpref_main_region_layout_theme_edit_spaces_pre(uiBlock *block, uiLayout *root, const char *name, ThemeSpace *space) {
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_TEXT, name, 0, 0, 4 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	uiDefBut(block, UI_BTYPE_SEPR, "()", 0, 0, 1 * UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	userpref_main_region_layout_theme_edit_spaces_post(block, root, space);
}

void userpref_main_region_layout_theme_edit_spaces(struct rContext *C, ARegion *region, uiBlock *block, uiLayout *root) {
	ScrArea *area = CTX_wm_area(C);
	SpaceUser *user = (SpaceUser *)area->spacedata.first;

	uiLayout *grid = UI_layout_grid(root, 3, false, false);

	userpref_main_region_layout_theme_edit_spaces_pre(block, grid, "Empty", &user->editing.space_empty);
	userpref_main_region_layout_theme_edit_spaces_pre(block, grid, "View3D", &user->editing.space_view3d);
	userpref_main_region_layout_theme_edit_spaces_pre(block, grid, "User Preferences", &user->editing.space_userpref);

	UI_block_layout_set_current(block, root);
}

void userpref_main_region_layout_theme_push_buttons(struct rContext *C, ARegion *region, uiBlock *block, uiLayout *root, Theme *theme) {
	uiLayout *grid = UI_layout_grid(root, 3, true, false);
	
	uiBut *but;
	but = uiDefBut(block, UI_BTYPE_PUSH, "Apply", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	UI_but_func_set(but, userpref_main_theme_update, theme, THEME_APPLY);
	but = uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	but = uiDefBut(block, UI_BTYPE_PUSH, "Reset", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	UI_but_func_set(but, userpref_main_theme_update, theme, THEME_RESET);
	
	UI_block_layout_set_current(block, root);
}

void userpref_main_region_layout_theme_push(struct rContext *C, ARegion *region, uiBlock *block, uiLayout *root) {
	ScrArea *area = CTX_wm_area(C);
	SpaceUser *user = (SpaceUser *)area->spacedata.first;

	uiLayout *grid = UI_layout_grid(root, 3, true, false);
	uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	userpref_main_region_layout_theme_push_buttons(C, region, block, grid, &user->editing);
	uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, UI_UNIT_X, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
	UI_block_layout_set_current(block, root);
}

void userpref_main_region_layout(struct rContext *C, ARegion *region) {
	ScrArea *area = CTX_wm_area(C);
	SpaceUser *user = (SpaceUser *)area->spacedata.first;

	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "USERPREF_theme_editor"))) {
		uiLayout *root = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROOT, region->sizex * region->hscroll, region->sizey * region->vscroll, 0, 0);

		uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		userpref_main_region_layout_theme_list(C, region, block, root);
		uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		uiDefBut(block, UI_BTYPE_TEXT, "Widgets", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		userpref_main_region_layout_theme_edit_widget(C, region, block, root);
		uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		uiDefBut(block, UI_BTYPE_TEXT, "Spaces", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		userpref_main_region_layout_theme_edit_spaces(C, region, block, root);
		uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);
		userpref_main_region_layout_theme_push(C, region, block, root);
		uiDefBut(block, UI_BTYPE_SEPR, "", 0, 0, region->sizex, UI_UNIT_Y, NULL, UI_POINTER_NIL, 0, 0);

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
