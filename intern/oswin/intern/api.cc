#include "oswin.h"
#include "oswin.hh"

using namespace rose::tiny_window;

WTKWindowManager *WTK_window_manager_new(void) {
	tWindowManager *manager = tWindowManager::Init(nullptr);
	return reinterpret_cast<WTKWindowManager *>(manager);
}

void WTK_window_manager_free(WTKWindowManager *vmanager) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		manager->Shutdown();
		delete manager;
	}
}
void WTK_window_manager_poll(WTKWindowManager *vmanager) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	manager->Poll();
}

bool WTK_window_manager_has_events(WTKWindowManager *vmanager) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	return manager->HasEventsWaiting();
}

bool WTK_set_clipboard(WTKWindowManager *vmanager, const char *buffer, unsigned int len, bool selection) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	return manager->SetClipboard(buffer, len, selection);
}
bool WTK_get_clipboard(WTKWindowManager *vmanager, char **r_buffer, unsigned int *r_len, bool selection) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	return manager->GetClipboard(r_buffer, r_len, selection);
}

void WTK_sleep(int ms) {
	tWindowManager::Sleep(ms);
}

void WTK_window_manager_destroy_callback(WTKWindowManager *vmanager, WTKDestroyCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->DestroyEvent = [=](tWindow *window) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), userdata);
		};
		/* clang-format on */
	}
}

void WTK_window_manager_resize_callback(WTKWindowManager *vmanager, WTKResizeCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->ResizeEvent = [=](tWindow *window, unsigned int cx, unsigned int cy) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), cx, cy, userdata);
		};
		/* clang-format on */
	}
}
void WTK_window_manager_move_callback(WTKWindowManager *vmanager, WTKMoveCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->MoveEvent = [=](tWindow *window, int x, int y) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), x, y, userdata);
		};
		/* clang-format on */
	}
}
void WTK_window_manager_activate_callback(WTKWindowManager *vmanager, WTKActivateCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->ActivateEvent = [=](tWindow *window, bool activate) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), activate, userdata);
		};
		/* clang-format on */
	}
}
void WTK_window_manager_mouse_callback(WTKWindowManager *vmanager, WTKMouseCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->MouseEvent = [=](tWindow *window, int x, int y, double time) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), x, y, time, userdata);
		};
		/* clang-format on */
	}
}
void WTK_window_manager_wheel_callback(WTKWindowManager *vmanager, WTKWheelCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->WheelEvent = [=](tWindow *window, int dx, int dy, double time) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), dx, dy, time, userdata);
		};
		/* clang-format on */
	}
}
void WTK_window_manager_button_down_callback(WTKWindowManager *vmanager, WTKButtonDownCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->ButtonDownEvent = [=](tWindow *window, int key, int x, int y, double time) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), key, x, y, time, userdata);
		};
		/* clang-format on */
	}
}
void WTK_window_manager_button_up_callback(WTKWindowManager *vmanager, WTKButtonUpCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->ButtonUpEvent = [=](tWindow *window, int key, int x, int y, double time) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), key, x, y, time, userdata);
		};
		/* clang-format on */
	}
}
void WTK_window_manager_key_down_callback(WTKWindowManager *vmanager, WTKKeyDownCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->KeyDownEvent = [=](tWindow *window, int key, bool repeat, char utf8[4], double time) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), key, repeat, utf8, time, userdata);
		};
		/* clang-format on */
	}
}
void WTK_window_manager_key_up_callback(WTKWindowManager *vmanager, WTKKeyUpCallbackFn fn, void *userdata) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	if (manager) {
		/* clang-format off */
		manager->KeyUpEvent = [=](tWindow *window, int key, double time) -> void {
			fn(reinterpret_cast<WTKWindow *>(window), key, time, userdata);
		};
		/* clang-format on */
	}
}

WTKWindow *WTK_create_window(WTKWindowManager *vmanager, const char *title, int width, int height) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);

	WindowSetting settings;
	settings.name = title;
	settings.width = width;
	settings.height = height;

	return reinterpret_cast<WTKWindow *>(manager->AddWindow(settings));
}

bool WTK_window_should_close(WTKWindow *vwindow) {
	tWindow *window = reinterpret_cast<tWindow *>(vwindow);
	return window->ShouldClose();
}

void WTK_window_free(WTKWindowManager *vmanager, WTKWindow *vwindow) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);
	tWindow *window = reinterpret_cast<tWindow *>(vwindow);
	manager->ShutdownWindow(window);
}

void WTK_window_set_swap_interval(WTKWindow *vwindow, int interval) {
	tWindow *window = reinterpret_cast<tWindow *>(vwindow);
	window->SetSwapInterval(interval);
}

void WTK_window_swap_buffers(WTKWindow *vwindow) {
	tWindow *window = reinterpret_cast<tWindow *>(vwindow);
	window->SwapBuffers();
}

void WTK_window_make_context_current(WTKWindow *vwindow) {
	tWindow *window = reinterpret_cast<tWindow *>(vwindow);
	window->MakeContextCurrent();
}

void WTK_window_pos(WTKWindow *vwindow, int *r_posx, int *r_posy) {
	tWindow *window = reinterpret_cast<tWindow *>(vwindow);
	*r_posx = window->posx;
	*r_posy = window->posy;
}

void WTK_window_size(WTKWindow *vwindow, int *r_sizex, int *r_sizey) {
	tWindow *window = reinterpret_cast<tWindow *>(vwindow);
	*r_sizex = window->clientx;
	*r_sizey = window->clienty;
}

void WTK_window_show(WTKWindow *vwindow) {
	tWindow *window = reinterpret_cast<tWindow *>(vwindow);
	return window->Show();
}
void WTK_window_hide(WTKWindow *vwindow) {
	tWindow *window = reinterpret_cast<tWindow *>(vwindow);
	return window->Hide();
}

bool WTK_window_is_minimized(const WTKWindow *vwindow) {
	const tWindow *window = reinterpret_cast<const tWindow *>(vwindow);
	return window->GetState() == State::minimized;
}
bool WTK_window_is_maximized(const WTKWindow *vwindow) {
	const tWindow *window = reinterpret_cast<const tWindow *>(vwindow);
	return window->GetState() == State::maximized;
}

double WTK_elapsed_time(WTKWindowManager *vmanager) {
	tWindowManager *manager = reinterpret_cast<tWindowManager *>(vmanager);

	return manager->GetElapsedTime();
}
