#include "glib.h"

#include "context.hh"
#include "platform.hh"
#include "window.hh"

void GHOST_Init() {
	InitPlatform();
}

void GHOST_Exit() {
	ClosePlatform();
}

float GHOST_GetTime32() {
	return (glib_platform) ? (float)glib_platform->GetTime() : 0.0f;
}
double GHOST_GetTime64() {
	return (glib_platform) ? (double)glib_platform->GetTime() : 0.0f;
}

GWindow *GHOST_InitWindow(GWindow *parent, int width, int height) {
	if (InitPlatform()) {
		return NULL;
	}

	WindowInterface *window = glib_platform->InitWindow(reinterpret_cast<WindowInterface *>(parent), width, height);
	return reinterpret_cast<GWindow *>(window);
}

void GHOST_CloseWindow(GWindow *window) {
	glib_platform->CloseWindow(reinterpret_cast<WindowInterface *>(window));
}

bool GHOST_IsWindow(GWindow *window) {
	if (glib_platform) {
		return glib_platform->IsWindow(reinterpret_cast<WindowInterface *>(window));
	}
	return false;
}

void GHOST_WindowSetUserData(GWindow *window, void *userdata) {
	reinterpret_cast<WindowInterface *>(window)->SetUserData(userdata);
}

void *GHOST_WindowGetUserData(const GWindow *window) {
	return reinterpret_cast<const WindowInterface *>(window)->GetUserData();
}

GWindow *GHOST_GetWindowUnderCursor(int x, int y) {
	WindowInterface *window = glib_platform->GetWindowUnderCursor(x, y);
	
	return reinterpret_cast<GWindow *>(window);
}

void GHOST_SetWindowCaptionRect(GWindow *window, int xmin, int xmax, int ymin, int ymax) {
	reinterpret_cast<WindowInterface *>(window)->SetWindowCaptionRect(xmin, xmax, ymin, ymax);
}

GSize GHOST_GetScreenSize() {
	return glib_platform->GetScreenSize();
}

GPosition GHOST_GetCursorPosition(GWindow *window) {
	return glib_platform->GetCursorPosition(reinterpret_cast<WindowInterface *>(window));
}

GPosition GHOST_ScreenToClient(GWindow *window, int x, int y) {
	return reinterpret_cast<const WindowInterface *>(window)->ScreenToClient(x, y);
}

GPosition GHOST_ClientToScreen(GWindow *window, int x, int y) {
	return reinterpret_cast<const WindowInterface *>(window)->ClientToScreen(x, y);
}

GSize GHOST_GetWindowSize(GWindow *window) {
	return reinterpret_cast<WindowInterface *>(window)->GetWindowSize();
}

GSize GHOST_GetClientSize(GWindow *window) {
	return reinterpret_cast<WindowInterface *>(window)->GetClientSize();
}

GPosition GHOST_GetWindowPos(GWindow *window) {
	return reinterpret_cast<WindowInterface *>(window)->GetPos();
}

void GHOST_SetWindowSize(GWindow *window, int x, int y) {
	GSize size = {x, y};
	reinterpret_cast<WindowInterface *>(window)->SetWindowSize(size);
}

void GHOST_SetClientSize(GWindow *window, int x, int y) {
	GSize size = {x, y};
	reinterpret_cast<WindowInterface *>(window)->SetClientSize(size);
}

void GHOST_SetWindowPos(GWindow *window, int x, int y) {
	GPosition position = {x, y};
	reinterpret_cast<WindowInterface *>(window)->SetPos(position);
}

void GHOST_GetWindowRect(GWindow *window, GRect *r_rect) {
	reinterpret_cast<WindowInterface *>(window)->GetWindowRect(r_rect);
}

void GHOST_GetClientRect(GWindow *window, GRect *r_rect) {
	reinterpret_cast<WindowInterface *>(window)->GetClientRect(r_rect);
}

bool GHOST_IsWindowMinimized(GWindow *window) {
	return reinterpret_cast<WindowInterface *>(window)->IsIconic();
}

GContext *GHOST_InstallWindowContext(GWindow *window, int type) {
	ContextInterface *context = reinterpret_cast<WindowInterface *>(window)->InstallContext(type);
	return reinterpret_cast<GContext *>(context);
}

GContext *GHOST_GetWindowContext(GWindow *window) {
	ContextInterface *context = reinterpret_cast<WindowInterface *>(window)->GetContext();
	return reinterpret_cast<GContext *>(context);
}

bool GHOST_ProcessEvents(bool wait) {
	return glib_platform->ProcessEvents(wait);
}

void GHOST_DispatchEvents() {
	if (glib_platform) {
		glib_platform->DispatchEvents();
	}
}

void GHOST_PostEvent(GWindow *wnd, int type, void *evtdata) {
	glib_platform->PostEvent(reinterpret_cast<WindowInterface *>(wnd), type, evtdata);
}

void GHOST_DispatchEvent(GWindow *wnd, int type, void *evtdata) {
	glib_platform->DispatchEvent(reinterpret_cast<WindowInterface *>(wnd), type, evtdata);
}

void GHOST_EventSubscribe(EventCallbackFn fn, void *userdata) {
	glib_platform->EventSubscribe(fn, userdata);
}

void GHOST_EventUnsubscribe(EventCallbackFn fn) {
	glib_platform->EventUnsubscribe(fn);
}

bool GHOST_ActivateWindowDrawingContext(GWindow *wnd) {
	ContextInterface *context = reinterpret_cast<WindowInterface *>(wnd)->GetContext();

	if (context) {
		return context->Activate();
	}
	return false;
}

bool GHOST_SwapWindowBuffers(GWindow *wnd) {
	ContextInterface *context = reinterpret_cast<WindowInterface *>(wnd)->GetContext();

	if (context) {
		return context->SwapBuffers();
	}
	return false;
}

bool GHOST_ActivateDrawingContext(GContext *context) {
	return reinterpret_cast<ContextInterface *>(context)->Activate();
}

bool GHOST_DeactivateDrawingContext(GContext *context) {
	return reinterpret_cast<ContextInterface *>(context)->Deactivate();
}

bool GHOST_SwapBuffers(GContext *context) {
	return reinterpret_cast<ContextInterface *>(context)->SwapBuffers();
}
