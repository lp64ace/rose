#ifndef GTK_X11_WINDOW_MANAGER_HH
#define GTK_X11_WINDOW_MANAGER_HH

#include "MEM_guardedalloc.h"

#include "intern/gtk_window_manager.hh"

#include "gtk_x11_window.hh"
#include "gtk_x11_render.hh"

class GTKManagerX11 final : public GTKManagerInterface {
public:
	GTKManagerX11();
	~GTKManagerX11();

	bool HasEvents();
	void Poll();

	bool SetClipboard(const char *buffer, unsigned int length, bool selection);
	bool GetClipboard(char **buffer, unsigned int *length, bool selection) const;

	GTKWindowX11 *GetWindowByHandle(XWindow window);
};

#endif

