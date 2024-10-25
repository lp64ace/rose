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

typedef struct WTKPlatform WTKPlatform;

/** Create a new platform based on the most suitable backend available. */
struct WTKPlatform *WTK_platform_init();
/** Destroy a platform and any resources that may have been allocated. */
void WTK_platform_exit(struct WTKPlatform *platform);

/** Return the number of display devices on this system. */
int WTK_display_count(struct WTKPlatform *platform);
/** Return the number of display settings for the specified display device. */
int WTK_display_settings_count(struct WTKPlatform *platform, int display);

/** Fetch the display information of the specified display setting. */
bool WTK_display_setting_get(struct WTKPlatform *platform, int display, int setting, WTKDisplaySetting *);
bool WTK_display_current_setting_get(struct WTKPlatform *platform, int display, WTKDisplaySetting *);
bool WTK_display_current_setting_set(struct WTKPlatform *platform, int display, WTKDisplaySetting *);

#ifdef __cplusplus
}
#endif

#endif // GHOST_LIB_H
