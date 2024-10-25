#include "MEM_guardedalloc.h"

#include "x11_platform.hh"

namespace ghost {

X11DisplayManagerBase::X11DisplayManagerBase(Display *display) : display_(display) {
}

bool X11DisplayManagerBase::init() {
	if(!display_) {
		return false;
	}
	
	return true;
}

int X11DisplayManagerBase::GetNumDisplays(void) const {
	/**
	 * NOTE: for this to work as documented, 
     * we would need to use Xinerama for code that did this, 
     * we've since removed since its not worth the extra dependency.
	 */
	return 1;
}
int X11DisplayManagerBase::GetNumDisplaySettings(int display) const {
	return 1;
}
bool X11DisplayManagerBase::GetDisplaySetting(int display, int setting, WTKDisplaySetting *info) {
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

bool X11DisplayManagerBase::GetCurrentDisplaySetting(int display, WTKDisplaySetting *info) {
	return GetDisplaySetting(display, 0, info);
}
bool X11DisplayManagerBase::SetCurrentDisplaySetting(int display, WTKDisplaySetting *info) {
	return false;
}

bool X11Platform::init() {
	if(!(this->display_ = XOpenDisplay(nullptr))) {
		return false;
	}
	
	this->display_mngr_ = MEM_new<X11DisplayManagerBase>("X11DisplayManagerBase", this->display_);
	if(!this->display_mngr_->init()) {
		MEM_delete<X11DisplayManagerBase>(reinterpret_cast<X11DisplayManagerBase *>(this->display_mngr_));
		return false;
	}
	
	return true;
}

X11Platform::~X11Platform() {
	MEM_delete<X11DisplayManagerBase>(reinterpret_cast<X11DisplayManagerBase *>(this->display_mngr_));
	
	XCloseDisplay(this->display_);
}

} // namespace ghost
