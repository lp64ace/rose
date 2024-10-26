#include "MEM_guardedalloc.h"

#include "win32_display.hh"
#include "win32_platform.hh"
#include "win32_window.hh"

namespace ghost {

bool WTK_platform_win32_init_wndclass(WNDCLASSEXA *wndclass) {
	memset(wndclass, 0, sizeof(WNDCLASSEXA));

	wndclass->cbSize = sizeof(WNDCLASSEXA);
	wndclass->style = CS_HREDRAW | CS_VREDRAW;
	wndclass->lpfnWndProc = Win32Platform::WndProc;
	wndclass->cbClsExtra = 0;
	wndclass->cbWndExtra = 0;
	wndclass->hInstance = ::GetModuleHandle(NULL);
	wndclass->hIcon = ::LoadIconA(NULL, IDI_APPLICATION);

	if (wndclass->hIcon) {
		wndclass->hIconSm = ::LoadIconA(NULL, IDI_APPLICATION);
	}
	wndclass->hCursor = ::LoadCursorA(0, IDC_ARROW);
	wndclass->hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wndclass->lpszMenuName = 0;
	wndclass->lpszClassName = Win32Platform::WndClassName;

	if (::RegisterClassExA(wndclass) == 0) {
		return false;
	}
	return true;
}

bool Win32Platform::init() {
	if (!WTK_platform_win32_init_wndclass(&this->wndclass_)) {
		return false;
	}

	this->display_mngr_ = MEM_new<Win32DisplayManager>("Win32DisplayManager");
	if(!this->display_mngr_->init()) {
		return false;
	}
	
	this->window_mngr_ = MEM_new<Win32WindowManager>("Win32WindowManager");
	if(!this->window_mngr_->init()) {
		return false;
	}
	
	return true;
}

bool Win32Platform::poll() {
	MSG msg;

	do {
		if (!::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
			break;
		}

		/** Process all the events waiting for us. */
		while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
			/* #TranslateMessage doesn't alter the message, and doesn't change our raw keyboard data.
			 * Needed for #MapVirtualKey or if we ever need to get chars from wm_ime_char or similar. */
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}
	} while (false);

	return true;
}

Win32Platform::~Win32Platform() {
	MEM_delete<Win32WindowManager>(reinterpret_cast<Win32WindowManager *>(this->window_mngr_));
	MEM_delete<Win32DisplayManager>(reinterpret_cast<Win32DisplayManager *>(this->display_mngr_));
}

const char *Win32Platform::WndClassName = "RoseWndClass";

LRESULT Win32Platform::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	LRESULT result = 0;

	Win32Window *window = reinterpret_cast<Win32Window *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (!window) {
		return DefWindowProcA(hWnd, message, wParam, lParam);
	}

	switch (message) {
		case WM_DESTROY: {
			window->exit();
		} break;
		default: {
			result = DefWindowProcA(hWnd, message, wParam, lParam);
		} break;
	}

	return result;
}

} // namespace ghost
