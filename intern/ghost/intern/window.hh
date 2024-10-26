#ifndef GHOST_WINDOW_HH
#define GHOST_WINDOW_HH

#include <algorithm>
#include <vector>

namespace ghost {

struct WindowBase {
	/** Change the visibility of the window, see the WTK_SW_XXX enum values. */
	virtual bool Show(int command) = 0;
	
	virtual bool GetPos(int *r_left, int *r_top) const = 0;
	virtual bool GetSize(int *r_width, int *r_height) const = 0;
	
	virtual bool SetPos(int left, int top) = 0;
	virtual bool SetSize(int width, int height) = 0;
	
	/** Ensure that any derived properties are freed as well. */
	virtual ~WindowBase() = default;
};

struct WindowManagerBase {
	virtual bool init() = 0;
	
	/** Create a new window, the new window will not be shown in the screen until #Show is called. */
	virtual struct WindowBase *Spawn(struct WindowBase *parent, const char *title, int x, int y, int w, int h) = 0;
	virtual bool Close(struct WindowBase *window);
	virtual bool IsValid(struct WindowBase *window);
	
	/** Ensure that any derived properties are freed as well. */
	virtual ~WindowManagerBase();
	
	std::vector<WindowBase *> windows_;
};

} // namespace ghost

#endif // GHOST_WINDOW_HH