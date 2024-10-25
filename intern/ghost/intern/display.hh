#ifndef GHOST_DISPLAY_HH
#define GHOST_DISPLAY_HH

#include "glib.h"

namespace ghost {

struct DisplayManagerBase {
	virtual bool init() = 0;
	
	/** Return the number of display devices on this system. */
	virtual int GetNumDisplays(void) const = 0;
	/** Return the number of display settings for the specified display device. */
	virtual int GetNumDisplaySettings(int display) const = 0;
	/** Fetch the display information of the specified display setting. */
	virtual bool GetDisplaySetting(int display, int setting, WTKDisplaySetting *) = 0;
	
	virtual bool GetCurrentDisplaySetting(int display, WTKDisplaySetting *) = 0;
	virtual bool SetCurrentDisplaySetting(int display, WTKDisplaySetting *) = 0;
	
	/** Ensure that any derived properties are freed as well. */
	virtual ~DisplayManagerBase() = default;
};

} // namespace ghost

#endif // GHOST_DISPLAY_HH