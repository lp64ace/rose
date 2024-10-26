#ifndef X11_WINDOW_HH
#define X11_WINDOW_HH

#include "intern/window.hh"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace ghost {

struct X11WindowManager;

struct X11Window : public WindowBase {
	X11Window(struct X11WindowManager *manager);
	
	bool init(struct X11Window *parent, const char *title, int x, int y, int w, int h);
	bool exit();
	
	void SetTitle(const char *title);
	
	/** Change the visibility of the window, see the WTK_SW_XXX enum values. */
	bool Show(int command) override;
	
	bool GetPos(int *r_left, int *r_top) const override;
	bool GetSize(int *r_width, int *r_height) const override;
	
	bool SetPos(int left, int top) override;
	bool SetSize(int width, int height) override;
	
	~X11Window();
	
	struct X11WindowManager *manager_;
	
	Window handle_;
};

struct X11WindowManager : public WindowManagerBase {
	X11WindowManager(Display *display);
	
	bool init() override;
	
	/** Create a new window, the new window will not be shown in the screen until #Show is called. */
	struct WindowBase *Spawn(struct WindowBase *parent, const char *title, int x, int y, int w, int h) override;
	struct X11Window *Find(Window handle);
	
	/** This is owned by #X11Platform, not ourselves. */
	Display *display_;
	
	struct {
		Atom WM_CHANGE_STATE;
		Atom WM_DELETE_WINDOW;
		
		Atom _NET_WM_STATE;
		Atom _NET_WM_STATE_MAXIMIZED_HORZ;
		Atom _NET_WM_STATE_MAXIMIZED_VERT;
		Atom _NET_WM_STATE_HIDDEN;
	};
};

} // namespace ghost

#endif // X11_WINDOW_HH
