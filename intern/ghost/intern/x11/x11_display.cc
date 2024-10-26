#include "MEM_guardedalloc.h"

#include "x11_display.hh"

namespace ghost {

X11DisplayManager::X11DisplayManager(Display *display) : display_(display) {
}

bool X11DisplayManager::init() {
	if(!display_) {
		return false;
	}
	
	return true;
}

int X11DisplayManager::GetNumDisplays(void) const {
	/**
	 * NOTE: for this to work as documented, 
     * we would need to use Xinerama for code that did this, 
     * we've since removed since its not worth the extra dependency.
	 */
	return 1;
}
int X11DisplayManager::GetNumDisplaySettings(int display) const {
	return 1;
}
bool X11DisplayManager::GetDisplaySetting(int display, int setting, WTKDisplaySetting *info) {
	if(!(display < GetNumDisplays())) {
		return false;
	}
	if(!(setting < GetNumDisplaySettings(display))) {
		return false;
	}
	info->xpixels = DisplayWidth(display_, DefaultScreen(display_));
	info->ypixels = DisplayHeight(display_, DefaultScreen(display_));
	info->bpp = DefaultDepth(display_, DefaultScreen(display_));
	info->framerate = 60.0f;
	return true;
}

bool X11DisplayManager::GetCurrentDisplaySetting(int display, WTKDisplaySetting *info) {
	return GetDisplaySetting(display, 0, info);
}
bool X11DisplayManager::SetCurrentDisplaySetting(int display, WTKDisplaySetting *info) {
	return false;
}

} // namespace ghost
