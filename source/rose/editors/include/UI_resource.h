#ifndef UI_RESOURCE_H
#define UI_RESOURCE_H

#ifdef __cplusplus
extern "C" {
#endif

struct Theme;

enum {
	TH_BACK,
	TH_TEXT,
	TH_TEXT_HI,
	
	TH_TAB_BACK,
	TH_TAB_OUTLINE,
};

typedef struct ThemeState {
	struct Theme *theme;
	
	int spacetype;
	int regionid;
} ThemeState;

struct Theme *UI_GetTheme();

void UI_GetThemeColor3fv(int colorid, float col[3]);
void UI_GetThemeColor4fv(int colorid, float col[4]);

void UI_SetTheme(int spacetype, int regionid);

#ifdef __cplusplus
}
#endif

#endif // UI_RESOURCE_H
