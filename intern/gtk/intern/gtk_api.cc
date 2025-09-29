#include "GTK_api.h"

#include "intern/gtk_render.hh"
#include "intern/gtk_window.hh"
#include "intern/gtk_window_manager.hh"

#ifdef WIN32
#	include "win32/gtk_win32_render.hh"
#	include "win32/gtk_win32_window.hh"
#	include "win32/gtk_win32_window_manager.hh"
#endif

GTKManagerInterface *GTK_window_manager_new_ex(int backend) {
	switch (backend) {
#ifdef WIN32
		case GTK_WINDOW_MANAGER_WIN32: {
			return static_cast<GTKManagerInterface *>(MEM_new<GTKManagerWin32>("GTKManagerWin32"));
		} break;
#endif
	}
	return NULL;
}

struct GTKWindowManager *GTK_window_manager_new(int backend) {
	GTKManagerInterface *manager = NULL;

	if ((manager = GTK_window_manager_new_ex(backend))) {
		return reinterpret_cast<GTKWindowManager *>(manager);
	}

	int order[] = {
		GTK_WINDOW_MANAGER_WIN32,
		GTK_WINDOW_MANAGER_WAYLAND,
		GTK_WINDOW_MANAGER_X11,
	};

	for (size_t index = 0; index < sizeof(order) / sizeof(order[0]); index++) {
		if ((manager = GTK_window_manager_new_ex(order[index]))) {
			return reinterpret_cast<GTKWindowManager *>(manager);
		}
	}

	return NULL;
}

void GTK_window_manager_free(struct GTKWindowManager *vmanager) {
	MEM_delete<GTKManagerInterface>(reinterpret_cast<GTKManagerInterface *>(vmanager));
}

bool GTK_window_manager_has_events(struct GTKWindowManager *vmanager) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		return manager->HasEvents();
	}

	return false;
}

void GTK_window_manager_poll(struct GTKWindowManager *vmanager) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		manager->Poll();
	}
}

double GTK_elapsed_time(struct GTKWindowManager *vmanager) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		return manager->Time();
	}

	return 0.0;
}

bool GTK_set_clipboard(struct GTKWindowManager *vmanager, const char *buffer, unsigned int len, bool selection) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		return manager->SetClipboard(buffer, len, selection);
	}

	return false;
}

bool GTK_get_clipboard(struct GTKWindowManager *vmanager, char **r_buffer, unsigned int *r_len, bool selection) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		return manager->GetClipboard(r_buffer, r_len, selection);
	}

	return false;
}

void GTK_window_manager_destroy_callback(struct GTKWindowManager *vmanager, GTKDestroyCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchDestroy = [=](GTKWindowInterface *window) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), userdata);
		};
		/* clang-format on */
	}
}

void GTK_window_manager_resize_callback(struct GTKWindowManager *vmanager, GTKResizeCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchResize = [=](GTKWindowInterface *window, int x, int y) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), x, y, userdata);
		};
		/* clang-format on */
	}
}

void GTK_window_manager_move_callback(struct GTKWindowManager *vmanager, GTKMoveCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchMove = [=](GTKWindowInterface *window, int x, int y) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), x, y, userdata);
		};
		/* clang-format on */
	}
}

void GTK_window_manager_activate_callback(struct GTKWindowManager *vmanager, GTKActivateCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchFocus = [=](GTKWindowInterface *window, bool focus) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), focus, userdata);
		};
		/* clang-format on */
	}
}

void GTK_window_manager_mouse_callback(struct GTKWindowManager *vmanager, GTKMouseCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchMouse = [=](GTKWindowInterface *window, int x, int y, float time) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), x, y, time, userdata);
		};
		/* clang-format on */
	}
}

void GTK_window_manager_wheel_callback(struct GTKWindowManager *vmanager, GTKWheelCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchWheel = [=](GTKWindowInterface *window, int x, int y, float time) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), x, y, time, userdata);
		};
		/* clang-format on */
	}
}

void GTK_window_manager_button_down_callback(struct GTKWindowManager *vmanager, GTKButtonDownCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchButtonDown = [=](GTKWindowInterface *window, int btn, int x, int y, float time) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), btn, x, y, time, userdata);
		};
		/* clang-format on */
	}
}

void GTK_window_manager_button_up_callback(struct GTKWindowManager *vmanager, GTKButtonUpCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchButtonUp = [=](GTKWindowInterface *window, int btn, int x, int y, float time) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), btn, x, y, time, userdata);
		};
		/* clang-format on */
	}
}

void GTK_window_manager_key_down_callback(struct GTKWindowManager *vmanager, GTKKeyDownCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchKeyDown = [=](GTKWindowInterface *window, int key, bool repeat, char utf8[4], float time) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), key, repeat, utf8, time, userdata);
		};
		/* clang-format on */
	}
}

void GTK_window_manager_key_up_callback(struct GTKWindowManager *vmanager, GTKKeyUpCallbackFn fn, void *userdata) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		/* clang-format off */
		manager->DispatchKeyUp = [=](GTKWindowInterface *window, int key, float time) -> void {
			fn(reinterpret_cast<GTKWindow *>(window), key, time, userdata);
		};
		/* clang-format on */
	}
}

struct GTKWindow *GTK_create_window_ex(struct GTKWindowManager *vmanager, const char *title, int width, int height, int state) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		GTKWindowInterface *window = manager->NewWindowEx(NULL, title, width, height, GTK_WINDOW_RENDER_OPENGL, state);

		return reinterpret_cast<GTKWindow *>(window);
	}

	return NULL;
}

struct GTKWindow *GTK_create_window(struct GTKWindowManager *vmanager, const char *title, int width, int height) {
	return GTK_create_window_ex(vmanager, title, width, height, GTK_STATE_RESTORED);
}

void GTK_window_free(struct GTKWindowManager *vmanager, struct GTKWindow *vwindow) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	if (manager) {
		manager->DestroyWindow(reinterpret_cast<GTKWindowInterface *>(vwindow));
	}
}

void GTK_window_set_swap_interval(struct GTKWindow *vwindow, int interval) {
	GTKWindowInterface *window = reinterpret_cast<GTKWindowInterface *>(vwindow);
	GTKRenderInterface *render = window->GetRenderInterface();

	if (!render) {
		return;
	}

	render->SwapInterval(interval);
}

void GTK_window_swap_buffers(struct GTKWindow *vwindow) {
	GTKWindowInterface *window = reinterpret_cast<GTKWindowInterface *>(vwindow);
	GTKRenderInterface *render = window->GetRenderInterface();

	if (!render) {
		return;
	}

	render->SwapBuffers();
}

void GTK_window_make_context_current(struct GTKWindow *vwindow) {
	GTKWindowInterface *window = reinterpret_cast<GTKWindowInterface *>(vwindow);
	GTKRenderInterface *render = window->GetRenderInterface();

	if (!render) {
		return;
	}

	render->MakeCurrent();
}

void GTK_window_pos(struct GTKWindow *vwindow, int *x, int *y) {
	GTKWindowInterface *window = reinterpret_cast<GTKWindowInterface *>(vwindow);

	return window->GetPos(x, y);
}

void GTK_window_size(struct GTKWindow *vwindow, int *w, int *h) {
	GTKWindowInterface *window = reinterpret_cast<GTKWindowInterface *>(vwindow);

	return window->GetSize(w, h);
}

void GTK_window_show(struct GTKWindow *vwindow) {
	GTKWindowInterface *window = reinterpret_cast<GTKWindowInterface *>(vwindow);

	window->Show();
}

void GTK_window_hide(struct GTKWindow *vwindow) {
	GTKWindowInterface *window = reinterpret_cast<GTKWindowInterface *>(vwindow);

	window->Hide();
}

bool GTK_window_is_minimized(const struct GTKWindow *vwindow) {
	const GTKWindowInterface *window = reinterpret_cast<const GTKWindowInterface *>(vwindow);

	return window->GetState() == GTK_STATE_MINIMIZED;
}

bool GTK_window_is_maximized(const struct GTKWindow *vwindow) {
	const GTKWindowInterface *window = reinterpret_cast<const GTKWindowInterface *>(vwindow);

	return window->GetState() == GTK_STATE_MAXIMIZED;
}

struct GTKRender *GTK_render_create_ex(struct GTKWindowManager *vmanager, int backend) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	return reinterpret_cast<GTKRender *>(manager->NewRender(backend));
}

struct GTKRender *GTK_render_create(struct GTKWindowManager *vmanager) {
	return GTK_render_create_ex(vmanager, GTK_WINDOW_RENDER_OPENGL);
}

void GTK_render_free(struct GTKWindowManager *vmanager, struct GTKRender *vrender) {
	GTKManagerInterface *manager = reinterpret_cast<GTKManagerInterface *>(vmanager);

	manager->DestroyRender(reinterpret_cast<GTKRenderInterface *>(vrender));
}

void GTK_render_set_swap_interval(struct GTKRender *vrender, int interval) {
	GTKRenderInterface *render = reinterpret_cast<GTKRenderInterface *>(vrender);

	render->SwapInterval(interval);
}

void GTK_render_swap_buffers(struct GTKRender *vrender) {
	GTKRenderInterface *render = reinterpret_cast<GTKRenderInterface *>(vrender);

	render->SwapBuffers();
}

void GTK_render_make_context_current(struct GTKRender *vrender) {
	GTKRenderInterface *render = reinterpret_cast<GTKRenderInterface *>(vrender);

	if (render) {
		render->MakeCurrent();
	}
}
