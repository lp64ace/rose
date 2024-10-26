#include "MEM_guardedalloc.h"

#include "glib.h"

#include "display.hh"
#include "platform.hh"
#include "window.hh"

WTKPlatform *WTK_platform_init() {
	WTKPlatform *platform = reinterpret_cast<WTKPlatform *>(ghost::InitMostSuitablePlatform());
	
	return platform;
}

void WTK_platform_exit(WTKPlatform *platform) {
	MEM_delete<ghost::PlatformBase>(reinterpret_cast<ghost::PlatformBase *>(platform));
}

bool WTK_platform_poll(WTKPlatform *vplatform) {
	ghost::PlatformBase *platform = reinterpret_cast<ghost::PlatformBase *>(vplatform);

	return platform->poll();
}

int WTK_display_count(WTKPlatform *vplatform) {
	ghost::PlatformBase *platform = reinterpret_cast<ghost::PlatformBase *>(vplatform);
	ghost::DisplayManagerBase *display_mngr = platform->display_mngr_;
	
	return display_mngr->GetNumDisplays();
}
int WTK_display_settings_count(WTKPlatform *vplatform, int display) {
	ghost::PlatformBase *platform = reinterpret_cast<ghost::PlatformBase *>(vplatform);
	ghost::DisplayManagerBase *display_mngr = platform->display_mngr_;
	
	return display_mngr->GetNumDisplaySettings(display);
}

bool WTK_display_setting_get(WTKPlatform *vplatform, int display, int setting, WTKDisplaySetting *info) {
	ghost::PlatformBase *platform = reinterpret_cast<ghost::PlatformBase *>(vplatform);
	ghost::DisplayManagerBase *display_mngr = platform->display_mngr_;
	
	return display_mngr->GetDisplaySetting(display, setting, info);
}
bool WTK_display_current_setting_get(WTKPlatform *vplatform, int display, WTKDisplaySetting *info) {
	ghost::PlatformBase *platform = reinterpret_cast<ghost::PlatformBase *>(vplatform);
	ghost::DisplayManagerBase *display_mngr = platform->display_mngr_;
	
	return display_mngr->GetCurrentDisplaySetting(display, info);
}
bool WTK_display_current_setting_set(WTKPlatform *vplatform, int display, WTKDisplaySetting *info) {
	ghost::PlatformBase *platform = reinterpret_cast<ghost::PlatformBase *>(vplatform);
	ghost::DisplayManagerBase *display_mngr = platform->display_mngr_;
	
	return display_mngr->SetCurrentDisplaySetting(display, info);
}

bool WTK_window_is_valid(struct WTKPlatform *vplatform, struct WTKWindow *vwindow) {
	ghost::PlatformBase *platform = reinterpret_cast<ghost::PlatformBase *>(vplatform);
	ghost::WindowBase *window = reinterpret_cast<ghost::WindowBase *>(vwindow);
	ghost::WindowManagerBase *window_mngr = platform->window_mngr_;

	return window_mngr->IsValid(window);
}

WTKWindow *WTK_window_spawn_ex(WTKPlatform *vplatform, WTKWindow *vparent, const char *title, int x, int y, int w, int h) {
	ghost::PlatformBase *platform = reinterpret_cast<ghost::PlatformBase *>(vplatform);
	ghost::WindowBase *parent = reinterpret_cast<ghost::WindowBase *>(vparent);
	ghost::WindowManagerBase *window_mngr = platform->window_mngr_;

	return reinterpret_cast<WTKWindow *>(window_mngr->Spawn(parent, title, x, y, w, h));
}
WTKWindow *WTK_window_spawn(WTKPlatform *vplatform, const char *title, int x, int y, int w, int h) {
	ghost::PlatformBase *platform = reinterpret_cast<ghost::PlatformBase *>(vplatform);
	ghost::WindowManagerBase *window_mngr = platform->window_mngr_;

	return reinterpret_cast<WTKWindow *>(window_mngr->Spawn(NULL, title, x, y, w, h));
}

bool WTK_window_show_ex(WTKWindow *vwindow, int command) {
	ghost::WindowBase *window = reinterpret_cast<ghost::WindowBase *>(vwindow);

	return window->Show(command);
}
bool WTK_window_show(WTKWindow *vwindow) {
	ghost::WindowBase *window = reinterpret_cast<ghost::WindowBase *>(vwindow);

	return window->Show(WTK_SW_DEFAULT);
}

bool WTK_window_size(WTKWindow *vwindow, int *r_wdith, int *r_height) {
	ghost::WindowBase *window = reinterpret_cast<ghost::WindowBase *>(vwindow);

	return window->GetSize(r_wdith, r_height);
}
bool WTK_window_pos(WTKWindow *vwindow, int *r_left, int *r_top) {
	ghost::WindowBase *window = reinterpret_cast<ghost::WindowBase *>(vwindow);

	return window->GetPos(r_left, r_top);
}
bool WTK_window_resize(WTKWindow *vwindow, int width, int height) {
	ghost::WindowBase *window = reinterpret_cast<ghost::WindowBase *>(vwindow);

	return window->SetSize(width, height);
}
bool WTK_window_move(WTKWindow *vwindow, int left, int r_top) {
	ghost::WindowBase *window = reinterpret_cast<ghost::WindowBase *>(vwindow);

	return window->SetPos(left, r_top);
}
