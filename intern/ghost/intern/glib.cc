#include "glib.h"

#include "context.hh"
#include "platform.hh"
#include "window.hh"

GWindow *InitWindow(GWindow *parent, int width, int height) {
	if (InitPlatform()) {
		return NULL;
	}
	
	WindowInterface *window = glib_platform->InitWindow(reinterpret_cast<WindowInterface *>(parent), width, height);
	return reinterpret_cast<GWindow *>(window);
}

void CloseWindow(GWindow *window) {
	glib_platform->CloseWindow(reinterpret_cast<WindowInterface *>(window));
}

bool IsWindow(GWindow *window) {
	if (glib_platform) {
		return glib_platform->IsWindow(reinterpret_cast<WindowInterface *>(window));
	}
	return false;
}

GSize GetWindowSize(GWindow *window) {
	return reinterpret_cast<WindowInterface *>(window)->GetWindowSize();
}

GSize GetClientSize(GWindow *window) {
	return reinterpret_cast<WindowInterface *>(window)->GetClientSize();
}

GPosition GetWindowPos(GWindow *window) {
	return reinterpret_cast<WindowInterface *>(window)->GetPos();
}

void SetWindowSize(GWindow *window, int x, int y) {
	GSize size = {x, y};
	reinterpret_cast<WindowInterface *>(window)->SetWindowSize(size);
}

void SetClientSize(GWindow *window, int x, int y) {
	GSize size = {x, y};
	reinterpret_cast<WindowInterface *>(window)->SetClientSize(size);
}

void SetWindowPosition(GWindow *window, int x, int y) {
	GPosition position = {x, y};
	reinterpret_cast<WindowInterface *>(window)->SetPos(position);
}

void GetWindowRectangle(GWindow *window, GRect *r_rect) {
	reinterpret_cast<WindowInterface *>(window)->GetWindowRect(r_rect);
}

void GetClientRectangle(GWindow *window, GRect *r_rect) {
	reinterpret_cast<WindowInterface *>(window)->GetClientRect(r_rect);
}

GContext *InstallWindowContext(GWindow *window, int type) {
	ContextInterface *context = reinterpret_cast<WindowInterface *>(window)->InstallContext(type);
	return reinterpret_cast<GContext *>(context);
}

GContext *GetWindowContext(GWindow *window) {
	ContextInterface *context = reinterpret_cast<WindowInterface *>(window)->GetContext();
	return reinterpret_cast<GContext *>(context);
}

bool ProcessEvents(bool wait) {
	return glib_platform->ProcessEvents(wait);
}

void DispatchEvents() {
	if (glib_platform) {
		glib_platform->DispatchEvents();
	}
}

void PostEvent(GWindow *wnd, int type, void *evtdata) {
	glib_platform->PostEvent(reinterpret_cast<WindowInterface *>(wnd), type, evtdata);
}

void DispatchEvent(GWindow *wnd, int type, void *evtdata) {
	glib_platform->DispatchEvent(reinterpret_cast<WindowInterface *>(wnd), type, evtdata);
}

void EventSubscribe(EventCallbackFn fn) {
	glib_platform->EventSubscribe(fn);
}

void EventUnsubscribe(EventCallbackFn fn) {
	glib_platform->EventUnsubscribe(fn);
}
