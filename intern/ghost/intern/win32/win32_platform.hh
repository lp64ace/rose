#ifndef WIN32_PLATFORM_HH
#define WIN32_PLATFORM_HH

#include "intern/display.hh"
#include "intern/platform.hh"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ghost {

struct Win32DisplayManagerBase : public DisplayManagerBase {
	bool init() override;
	
	/** Return the number of display devices on this system. */
	int GetNumDisplays(void) const override;
	/** Return the number of display settings for the specified display device. */
	int GetNumDisplaySettings(int display) const override;
	/** Fetch the display information of the specified display setting. */
	bool GetDisplaySetting(int display, int setting, WTKDisplaySetting *) override;
	
	bool GetCurrentDisplaySetting(int display, WTKDisplaySetting *) override;
	bool SetCurrentDisplaySetting(int display, WTKDisplaySetting *) override;
};

struct Win32Platform : public PlatformBase {
	bool init() override;
	
	~Win32Platform();
};

} // namespace ghost

#endif // WIN32_PLATFORM_HH
