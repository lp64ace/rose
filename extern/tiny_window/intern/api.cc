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

void WTK_window_manager_resize_callback(WTKWindowManager *vmanager, WTKResizeCallbackFn fn, void *userdata) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	if (manager) {
		manager->resizeEvent = [=](TinyWindow::tWindow *window, TinyWindow::vec2_t<unsigned int> resolution) -> void {
			fn(vmanager, reinterpret_cast<WTKWindow *>(window), EVT_RESIZE, (int)resolution.x, (int)resolution.y, userdata);
		};
	}
}
void WTK_window_manager_move_callback(WTKWindowManager *vmanager, WTKMoveCallbackFn fn, void *userdata) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	if (manager) {
		manager->movedEvent = [=](TinyWindow::tWindow *window, TinyWindow::vec2_t<int> position) -> void {
			fn(vmanager, reinterpret_cast<WTKWindow *>(window), EVT_MOVED, position.x, position.y, userdata);
		};
	}
}

WTKWindow *WTK_create_window(WTKWindowManager *vmanager, const char *title, int width, int height) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	TinyWindow::windowSetting_t settings = {};
	settings.name = title;
	settings.resolution = TinyWindow::vec2_t<unsigned int>(width, height);
	settings.SetProfile(TinyWindow::profile_t::core);
	settings.currentState = TinyWindow::state_t::normal;
	settings.versionMajor = 4;
	settings.versionMinor = 3;
	
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

void WTK_window_set_swap_interval(WTKWindowManager *vmanager, WTKWindow *vwindow, int interval) {
	TinyWindow::windowManager *manager = reinterpret_cast<TinyWindow::windowManager *>(vmanager);
	TinyWindow::tWindow *window = reinterpret_cast<TinyWindow::tWindow *>(vwindow);
	manager->SetWindowSwapInterval(window, interval);
}

void WTK_window_swap_buffers(WTKWindow *vwindow) {
	TinyWindow::tWindow *window = reinterpret_cast<TinyWindow::tWindow *>(vwindow);
	window->SwapDrawBuffers();
}

void WTK_window_make_context_current(WTKWindow *vwindow) {
	TinyWindow::tWindow *window = reinterpret_cast<TinyWindow::tWindow *>(vwindow);
	window->MakeCurrentContext();
}

void WTK_window_pos(struct WTKWindow *vwindow, int *r_posx, int *r_posy) {
	TinyWindow::tWindow *window = reinterpret_cast<TinyWindow::tWindow *>(vwindow);
	*r_posx = window->position.x;
	*r_posy = window->position.y;
}

void WTK_window_size(struct WTKWindow *vwindow, int *r_sizex, int *r_sizey) {
	TinyWindow::tWindow *window = reinterpret_cast<TinyWindow::tWindow *>(vwindow);
	*r_sizex = window->settings.resolution.x;
	*r_sizey = window->settings.resolution.y;
}
