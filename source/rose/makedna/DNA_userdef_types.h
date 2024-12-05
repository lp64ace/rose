#include "DNA_listbase.h"

#ifndef DNA_USERDEF_TYPES_H
#	define DNA_USERDEF_TYPES_H

#	ifdef __cplusplus
extern "C" {
#	endif

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
	ThemeSpace space_view3d;
	ThemeSpace space_topbar;
	ThemeSpace space_statusbar;
	ThemeSpace space_userpref;
} Theme;

typedef struct UserDef {
	ListBase themes;
} UserDef;

extern UserDef U;

#	ifdef __cplusplus
}
#	endif

#endif	// DNA_USERDEF_TYPES_H
