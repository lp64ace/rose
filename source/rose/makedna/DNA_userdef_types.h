#ifndef DNA_USERDEF_TYPES_H
#define DNA_USERDEF_TYPES_H

#include "DNA_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uiWidgetColors {
	unsigned char outline[4];
	unsigned char inner[4];
	/** The color that will be used in the background for item selections! */
	unsigned char inner_sel[4];
	unsigned char text[4];
	/** The color that will be used in the background for text selections! */
	unsigned char text_sel[4];

	float roundness;
} uiWidgetColors;

typedef struct ThemeUI {
	uiWidgetColors wcol_sepr;
	uiWidgetColors wcol_but;
	uiWidgetColors wcol_txt;
	uiWidgetColors wcol_edit;
	uiWidgetColors wcol_scroll;

	/** The color that will be used for the text cursor. */
	unsigned char text_cur[4];
} ThemeUI;

typedef struct ThemeSpace {
	unsigned char back[4];
	unsigned char text[4];
	unsigned char text_hi[4];
	unsigned char header[4];
	unsigned char header_hi[4];
} ThemeSpace;

typedef struct Theme {
	struct Theme *prev, *next;

	char name[64];

	ThemeUI tui;

	ThemeSpace space_empty;
	ThemeSpace space_file;
	ThemeSpace space_view3d;
	ThemeSpace space_topbar;
	ThemeSpace space_statusbar;
	ThemeSpace space_properties;
} Theme;

typedef struct UserDef {
	char engine[64];

	int flag_ui;

	float view_rotate_sensitivity_turntable;

	ListBase themes;
} UserDef;

/** #UserDef->flag_ui */
enum {
	/**
	 * Controls whether menus close when clicking outside or when the mouse leaves the region:
	 *  - ON  | The menu region will exit when the mouse leaves the region.
	 *  - OFF | The menu region will exit only on mouse button press outside the region.
	 */
	UI_MENU_EXIT_AUTO = 1 << 0,
};

extern UserDef U;

#ifdef __cplusplus
}
#endif

#endif	// DNA_USERDEF_TYPES_H
