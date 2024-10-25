#include "MEM_guardedalloc.h"

#include "glib.h"

#include "display.hh"
#include "platform.hh"

WTKPlatform *WTK_platform_init() {
	WTKPlatform *platform = reinterpret_cast<WTKPlatform *>(ghost::InitMostSuitablePlatform());
	
	return platform;
}

void WTK_platform_exit(WTKPlatform *platform) {
	MEM_delete<ghost::PlatformBase>(reinterpret_cast<ghost::PlatformBase *>(platform));
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
