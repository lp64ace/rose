#ifndef X11_PLATFORM_HH
#define X11_PLATFORM_HH

#include "intern/platform.hh"

#include <X11/Xlib.h>

namespace ghost {

struct X11Platform : public PlatformBase {
	bool init() override;
	bool poll() override;
	
	~X11Platform();
	
	Display *display_ = NULL;
	
	void ClientMsgProc(XEvent *e, struct X11Window *window);
	void MsgProc(XEvent *e);
};

} // namespace ghost

#endif // X11_PLATFORM_HH
