#ifndef GTK_WINDOW_MANAGER_HH
#define GTK_WINDOW_MANAGER_HH

#include "GTK_window_type.h"

#include <chrono>
#include <functional>
#include <vector>

class GTKWindowInterface;
class GTKRenderInterface;

struct FormatSetting;

using FocusEventFn = std::function<void(GTKWindowInterface *window, bool)>;
using MoveEventFn = std::function<void(GTKWindowInterface *window, int x, int y)>;
using ResizeEventFn = std::function<void(GTKWindowInterface *window, int w, int h)>;
using MouseEventFn = std::function<void(GTKWindowInterface *window, int x, int y, float time)>;
using WheelEventFn = std::function<void(GTKWindowInterface *window, int x, int y, float time)>;
using ButtonDownEventFn = std::function<void(GTKWindowInterface *window, int btn, int x, int y, float time)>;
using ButtonUpEventFn = std::function<void(GTKWindowInterface *window, int btn, int x, int y, float time)>;
using KeyDownEventFn = std::function<void(GTKWindowInterface *window, int key, bool repeat, char utf8[4], float time)>;
using KeyUpEventFn = std::function<void(GTKWindowInterface *window, int key, float time)>;
using DestroyEventFn = std::function<void(GTKWindowInterface *window)>;

class GTKManagerInterface {
	std::vector<GTKRenderInterface *> renders;
	std::vector<GTKWindowInterface *> windows;

	std::chrono::system_clock::time_point t0;

public:
	GTKManagerInterface();
	virtual ~GTKManagerInterface();
	
	FocusEventFn DispatchFocus;
	MoveEventFn DispatchMove;
	ResizeEventFn DispatchResize;
	MouseEventFn DispatchMouse;
	WheelEventFn DispatchWheel;
	ButtonDownEventFn DispatchButtonDown;
	ButtonUpEventFn DispatchButtonUp;
	KeyDownEventFn DispatchKeyDown;
	KeyUpEventFn DispatchKeyUp;
	DestroyEventFn DispatchDestroy;
	
	GTKWindowInterface *NewWindowEx(GTKWindowInterface *parent, const char *title, int width, int height, int backend, int state);
	GTKWindowInterface *NewWindow(GTKWindowInterface *parent, const char *title, int width, int height, int backend);
	GTKRenderInterface *NewRender(int backend);
	
	void DestroyWindow(GTKWindowInterface *window);
	void DestroyRender(GTKRenderInterface *render);

	const std::vector<GTKWindowInterface *> &GetWindows() const;
	
	virtual bool HasEvents(void) = 0;
	virtual void Poll(void) = 0;
	virtual double Time(void) const;

	virtual bool SetClipboard(const char *buffer, unsigned int length, bool selection) {
		return false;
	}
	virtual bool GetClipboard(char **buffer, unsigned int *length, bool selection) const {
		return false;
	}
	
protected:
	virtual GTKWindowInterface *AllocateWindow(GTKManagerInterface *manager) = 0;
	
	void FreeWindow(GTKWindowInterface *window);
	void FreeRender(GTKRenderInterface *render);

	static FormatSetting *GetClosestFormat(const std::vector<FormatSetting *> &list, const FormatSetting *format);
};

#endif // GTK_WINDOW_MANAGER_HH
