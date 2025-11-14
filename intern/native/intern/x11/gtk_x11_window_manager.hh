#ifndef GTK_X11_WINDOW_MANAGER_HH
#define GTK_X11_WINDOW_MANAGER_HH

#include "MEM_guardedalloc.h"

#include "intern/gtk_window_manager.hh"

#include "gtk_x11_window.hh"
#include "gtk_x11_render.hh"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>

#include <X11/keysym.h>

#ifdef WITH_X11_XINPUT
/* I fucking hate linux GUI applications <3 */
#	include <X11/extensions/XInput2.h>
#	define USE_XINPUT_HOTPLUG
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>

class GTKManagerX11 final : public GTKManagerInterface {
	Display *display;
	
	XkbDescRec *xkb_descr;
	int keycode_last_repeat_key;

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
	XIM xim;

	/* Separe function from the constructor since it may not always be available. */
	bool XImmediateInit();
#endif

	bool XExtensionsInit();

public:
	GTKManagerX11();
	~GTKManagerX11();

	bool HasEvents();
	void Poll();
	
	GTKWindowX11 *GetWindowByHandle(Window window);
	
	bool SetClipboard(const char *buffer, unsigned int length, bool selection);
	bool GetClipboard(char **buffer, unsigned int *length, bool selection) const;

public:

	PFNGLXCREATECONTEXTATTRIBSARBPROC glxCreateContextAttribsARB;
	PFNGLXMAKECONTEXTCURRENTPROC glxMakeContextCurrent;
	PFNGLXSWAPINTERVALEXTPROC glxSwapIntervalEXT;
	PFNGLXCHOOSEFBCONFIGPROC glxChooseFBConfig;

protected:
	inline GTKWindowInterface *AllocateWindow(GTKManagerInterface *manager) {
		return static_cast<GTKWindowInterface *>(MEM_new<GTKWindowX11>("GTKWindowX11", static_cast<GTKManagerInterface *>(manager)));
	}
	
	friend class GTKWindowX11;
};

#endif

