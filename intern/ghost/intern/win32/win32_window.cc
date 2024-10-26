#include "MEM_guardedalloc.h"

#include "win32_platform.hh"
#include "win32_window.hh"

#include "glib.h"

namespace ghost {

Win32Window::Win32Window(Win32WindowManager *manager) : manager_(manager), hwnd_(NULL) {
}

bool Win32Window::init(Win32Window *parent, const char *title, int x, int y, int width, int height) {
	DWORD extended = (parent) ? WS_EX_APPWINDOW : 0;
	DWORD style = WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_OVERLAPPED | WS_SIZEBOX;
	this->hwnd_ = ::CreateWindowEx(
		extended,
		(LPCSTR)Win32Platform::WndClassName,
		(LPCSTR)title,
		style,
		(x == 0xffff) ? CW_USEDEFAULT : x,
		(y == 0xffff) ? CW_USEDEFAULT : y,
		width,
		height,
		(parent) ? parent->hwnd_ : HWND_DESKTOP,
		NULL,
		::GetModuleHandle(NULL),
		0
	);

	if (!this->hwnd_) {
		return false;
	}

	::SetWindowLongPtr(this->hwnd_, GWLP_USERDATA, (LONG_PTR)this);
	return true;
}

Win32Window::~Win32Window() {
	if (::GetWindowLongPtr(this->hwnd_, GWLP_USERDATA) == (LONG_PTR)this) {
		/** Make us unreachable from WndProc so no more messages will be dispatched for us! */
		::SetWindowLongPtr(this->hwnd_, GWLP_USERDATA, (LONG_PTR)NULL);
		::DestroyWindow(this->hwnd_);
	}
}

bool Win32Window::exit() {
	if (this->manager_) {
		/** Beware, this will also destroy the window! */
		return this->manager_->Close(this);
	}
	return false;
}

bool Win32Window::Show(int command) {
	switch (command) {
		case WTK_SW_DEFAULT:
		case WTK_SW_SHOW: {
			return ::ShowWindow(this->hwnd_, SW_SHOW);
		}
		case WTK_SW_HIDE: {
			return ::ShowWindow(this->hwnd_, SW_HIDE);
		}
		case WTK_SW_MINIMIZE: {
			return ::ShowWindow(this->hwnd_, SW_MINIMIZE);
		}
		case WTK_SW_MAXIMIZE: {
			return ::ShowWindow(this->hwnd_, SW_MAXIMIZE);
		}
	}

	return false;
}

bool Win32Window::GetPos(int *r_left, int *r_top) const {
	RECT window;
	if (!::GetWindowRect(this->hwnd_, &window)) {
		return false;
	}
	*r_left = window.left;
	*r_top = window.top;
	return true;
}
bool Win32Window::GetSize(int *r_width, int *r_height) const {
	RECT client;
	if (!::GetClientRect(this->hwnd_, &client)) {
		return false;
	}
	*r_width = client.right - client.left;
	*r_height = client.bottom - client.top;
	return true;
}

bool Win32Window::SetPos(int left, int top) {
	if (!::SetWindowPos(this->hwnd_, NULL, left, top, 0, 0, SWP_NOZORDER | SWP_NOSIZE)) {
		return false;
	}
	return true;
}
bool Win32Window::SetSize(int width, int height) {
	RECT window;
	if (!::GetWindowRect(this->hwnd_, &window)) {
		return false;
	}
	RECT client;
	if (!::GetClientRect(this->hwnd_, &client)) {
		return false;
	}
	int dx = (window.right - window.left) - (client.right - client.left);
	int dy = (window.bottom - window.top) - (client.bottom - client.top);
	if (!::SetWindowPos(this->hwnd_, NULL, 0, 0, width + dx, height + dy, SWP_NOZORDER | SWP_NOMOVE)) {
		return false;
	}
	return true;
}

bool Win32WindowManager::init() {
	return true;
}

WindowBase *Win32WindowManager::Spawn(WindowBase *vparent, const char *title, int x, int y, int width, int height) {
	Win32Window *parent = reinterpret_cast<Win32Window *>(vparent);
	Win32Window *window = MEM_new<Win32Window>("Win32Window", this);
	if (!window->init(parent, title, x, y, width, height)) {
		MEM_delete<Win32Window>(window);
		return NULL;
	}
	WindowBase *vwindow = reinterpret_cast<WindowBase *>(window);
	this->windows_.push_back(vwindow);
	return vwindow;
}

}  // namespace ghost
