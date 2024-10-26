#ifndef WIN32_WINDOW_HH
#define WIN32_WINDOW_HH

#include "intern/window.hh"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ghost {

struct Win32WindowManager;

struct Win32Window : public WindowBase {
	Win32Window(struct Win32WindowManager *manager);

	bool init(Win32Window *parent, const char *title, int x, int y, int width, int height);
	bool exit();

	/** Change the visibility of the window, see the WTK_SW_XXX enum values. */
	bool Show(int command) override;
	
	bool GetPos(int *r_left, int *r_top) const override;
	bool GetSize(int *r_width, int *r_height) const override;
	
	bool SetPos(int left, int top) override;
	bool SetSize(int width, int height) override;
	
	~Win32Window();

	struct Win32WindowManager *manager_;

	HWND hwnd_;
};

struct Win32WindowManager : public WindowManagerBase {
	bool init() override;
	
	/** Create a new window, the new window will not be shown in the screen until #Show is called. */
	WindowBase *Spawn(WindowBase *parent, const char *title, int x, int y, int width, int height) override;
};

} // namespace ghost

#endif // WIN32_WINDOW_HH
