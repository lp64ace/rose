#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_userdef_types.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "UI_resource.h"

static ThemeState theme_state = {
	.theme = NULL,
	.spacetype = SPACE_EMPTY,
	.regionid = RGN_TYPE_ANY,
};

const unsigned char *UI_GetThemeColorPtr(Theme *theme, int spacetype, int color) {
	ThemeSpace *ts = NULL;
	
	static unsigned char error[4] = {240, 0, 240, 255};
	
	if(theme) {
		const unsigned char *cp = error;
		
		switch(spacetype) {
			case SPACE_VIEW3D: {
				ts = &theme->space_view3d;
			} break;
			case SPACE_TOPBAR: {
				ts = &theme->space_topbar;
			} break;
			case SPACE_STATUSBAR: {
				ts = &theme->space_statusbar;
			} break;
			default: {
				ROSE_assert_unreachable();
			} break;
		}
		
		switch(color) {
			case TH_BACK: {
				if(ELEM(theme_state.regionid, RGN_TYPE_WINDOW)) {
					cp = ts->back;
				}
				else if(ELEM(theme_state.regionid, RGN_TYPE_HEADER)) {
					cp = ts->header;
				}
				else {
					ROSE_assert_unreachable();
				}
			} break;
			case TH_TEXT: {
				cp = ts->text;
			} break;
			case TH_TEXT_HI: {
				cp = ts->text_hi;
			} break;
		}
		
		return cp;
	}
	
	return NULL;
}

Theme *UI_GetTheme() {
	return theme_state.theme;
}

void UI_SetTheme(int spacetype, int regionid) {
	if(spacetype) {
		theme_state.theme = (Theme *)U.themes.first;
		theme_state.spacetype = spacetype;
		theme_state.regionid = regionid;
	}
	else if (regionid) {
		/** popups */
		theme_state.theme = (Theme *)U.themes.first;
		theme_state.spacetype = SPACE_TOPBAR;
		theme_state.regionid = regionid;
	}
}

void UI_GetThemeColor3fv(int colorid, float col[3]) {
	const unsigned char *cp = UI_GetThemeColorPtr(theme_state.theme, theme_state.spacetype, colorid);
    col[0] = ((float)cp[0]) / 255.0f;
    col[1] = ((float)cp[1]) / 255.0f;
    col[2] = ((float)cp[2]) / 255.0f;
}
void UI_GetThemeColor4fv(int colorid, float col[4]) {
	const unsigned char *cp = UI_GetThemeColorPtr(theme_state.theme, theme_state.spacetype, colorid);
	col[0] = ((float)cp[0]) / 255.0f;
    col[1] = ((float)cp[1]) / 255.0f;
    col[2] = ((float)cp[2]) / 255.0f;
    col[3] = ((float)cp[3]) / 255.0f;
}
