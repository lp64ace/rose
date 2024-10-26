#ifndef GHOST_LIB_H
#define GHOST_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WTKDisplaySetting {
	int xpixels;
	int ypixels;
	int bpp;
	int framerate;
} WTKDisplaySetting;

enum {
	WTK_SW_DEFAULT = 0,
	WTK_SW_SHOW,
	WTK_SW_HIDE,
	WTK_SW_MINIMIZE,
	WTK_SW_MAXIMIZE,
};

typedef struct WTKPlatform WTKPlatform;
typedef struct WTKWindow WTKWindow;

/** Create a new platform based on the most suitable backend available. */
struct WTKPlatform *WTK_platform_init();
/** Destroy a platform and any resources that may have been allocated. */
void WTK_platform_exit(struct WTKPlatform *platform);
bool WTK_platform_poll(struct WTKPlatform *platform);

/** Return the number of display devices on this system. */
int WTK_display_count(struct WTKPlatform *platform);
/** Return the number of display settings for the specified display device. */
int WTK_display_settings_count(struct WTKPlatform *platform, int display);

/** Fetch the display information of the specified display setting. */
bool WTK_display_setting_get(struct WTKPlatform *platform, int display, int setting, WTKDisplaySetting *);
bool WTK_display_current_setting_get(struct WTKPlatform *platform, int display, WTKDisplaySetting *);
bool WTK_display_current_setting_set(struct WTKPlatform *platform, int display, WTKDisplaySetting *);

bool WTK_window_is_valid(struct WTKPlatform *platform, struct WTKWindow *window);

struct WTKWindow *WTK_window_spawn_ex(struct WTKPlatform *platform, struct WTKWindow *parent, const char *title, int x, int y, int w, int h);
struct WTKWindow *WTK_window_spawn(struct WTKPlatform *platform, const char *title, int x, int y, int w, int h);

bool WTK_window_show_ex(struct WTKWindow *window, int command);
bool WTK_window_show(struct WTKWindow *window);
bool WTK_window_size(struct WTKWindow *window, int *r_wdith, int *r_height);
bool WTK_window_pos(struct WTKWindow *window, int *r_left, int *r_top);
bool WTK_window_resize(struct WTKWindow *window, int width, int height);
bool WTK_window_move(struct WTKWindow *window, int left, int r_top);

#ifdef __cplusplus
}
#endif

#endif // GHOST_LIB_H
