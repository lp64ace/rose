#include "tiny_window.h"
#include "tiny_window.hh"

WTKWindowManager *WTK_window_manager_new(void) {
	TinyWindow::windowManager *manager =  new TinyWindow::windowManager();
	return reinterpret_cast<WTKWindowManager *>(manager);
}

void WTK_window_manager_free(WTKWindowManager *vmanager) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	if(manager) {
		manager->ShutDown();
	}
}
void WTK_window_manager_poll(WTKWindowManager *vmanager) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	manager->PollForEvents();
}

void WTK_window_manager_window_callback(WTKWindowManager *vmanager, int event, WTKWindowCallbackFn fn, void *userdata) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	if (manager) {
		switch (event) {
			case EVT_DESTROY: {
				manager->destroyedEvent = [=](TinyWindow::tWindow *window) -> void {
					fn(vmanager, reinterpret_cast<WTKWindow *>(window), EVT_DESTROY, userdata);
				};
			} break;
			case EVT_MINIMIZED: {
				manager->minimizedEvent = [=](TinyWindow::tWindow *window) -> void {
					fn(vmanager, reinterpret_cast<WTKWindow *>(window), EVT_MINIMIZED, userdata);
				};
			} break;
			case EVT_MAXIMIZED: {
				manager->maximizedEvent = [=](TinyWindow::tWindow *window) -> void {
					fn(vmanager, reinterpret_cast<WTKWindow *>(window), EVT_MAXIMIZED, userdata);
				};
			} break;
		}
	}
}

WTKWindow *WTK_create_window(WTKWindowManager *vmanager, const char *title, int width, int height) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	TinyWindow::windowSetting_t settings = {};
	settings.name = title;
	settings.resolution = TinyWindow::vec2_t<unsigned int>(width, height);
	settings.SetProfile(TinyWindow::profile_t::core);
	settings.currentState = TinyWindow::state_t::normal;
	
	return reinterpret_cast<WTKWindow *>(manager->AddWindow(settings));
}

bool WTK_window_should_close(WTKWindow *vwindow) {
	TinyWindow::tWindow *window = reinterpret_cast<TinyWindow::tWindow *>(vwindow);
	return window->shouldClose;
}

void WTK_window_free(WTKWindowManager *vmanager, WTKWindow *vwindow) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	TinyWindow::tWindow *window = reinterpret_cast<TinyWindow::tWindow *>(vwindow);
	manager->RemoveWindow(window);
}
