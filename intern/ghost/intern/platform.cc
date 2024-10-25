#include "MEM_guardedalloc.h"

#include "platform.hh"

#if HAS_WIN32
#	include "win32/win32_platform.hh"
#endif
#if HAS_X11
#	include "x11/x11_platform.hh"
#endif

#include <stdio.h>

namespace ghost {

PlatformBase *InitMostSuitablePlatform(void) {
	PlatformBase *platform = NULL;
	
#if HAS_WIN32
	if(platform == NULL && (platform = MEM_new<Win32Platform>("Win32Platform"))) {
		if(!platform->init()) {
			/** Try more platforms and see if there is any that can init. */
			MEM_delete<Win32Platform>(static_cast<Win32Platform *>(platform));
			platform = NULL;
		}
	}
#endif
#if HAS_X11
	if(platform == NULL && (platform = MEM_new<X11Platform>("X11Platform"))) {
		if(!platform->init()) {
			/** Try more platforms and see if there is any that can init. */
			MEM_delete<X11Platform>(static_cast<X11Platform *>(platform));
			platform = NULL;
		}
	}
#endif

	return platform;
}

} // namespace ghost
