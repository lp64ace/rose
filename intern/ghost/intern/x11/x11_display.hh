#ifndef X11_DISPLAY_HH
#define X11_DISPLAY_HH

#include "intern/display.hh"

#include <X11/Xlib.h>

namespace ghost {

struct X11DisplayManager : public DisplayManagerBase {
	X11DisplayManager(Display *display);
	
	bool init() override;
	
	/** Return the number of display devices on this system. */
	int GetNumDisplays(void) const override;
	/** Return the number of display settings for the specified display device. */
	int GetNumDisplaySettings(int display) const override;
	/** Fetch the display information of the specified display setting. */
	bool GetDisplaySetting(int display, int setting, WTKDisplaySetting *) override;
	
	bool GetCurrentDisplaySetting(int display, WTKDisplaySetting *) override;
	bool SetCurrentDisplaySetting(int display, WTKDisplaySetting *) override;
	
	/** This is owned by #X11Platform, not ourselves. */
	Display *display_;
};

} // namespace ghost

#endif // X11_DISPLAY_HH
