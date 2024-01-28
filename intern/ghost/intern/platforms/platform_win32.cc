#include <gl/glew.h>
#include <gl/wglew.h>
#include <windowsx.h>

#include <algorithm>

#include "MEM_alloc.h"

#include "platform_win32.hh"

static class WindowsPlatform *glib_windows_platform = nullptr;

static bool glib_windows_register_window_class_ex(WNDCLASSEXA *wndclass) {
	memset(wndclass, 0, sizeof(WNDCLASSEXA));

	wndclass->cbSize = sizeof(WNDCLASSEXA);
	wndclass->style = CS_HREDRAW | CS_VREDRAW;
	wndclass->lpfnWndProc = WindowsPlatform::WndProc;
	wndclass->cbClsExtra = 0;
	wndclass->cbWndExtra = 0;
	wndclass->hInstance = ::GetModuleHandle(NULL);
	wndclass->hIcon = ::LoadIconA(nullptr, IDI_APPLICATION);

	if (wndclass->hIcon) {
		wndclass->hIconSm = ::LoadIconA(nullptr, IDI_APPLICATION);
	}
	wndclass->hCursor = ::LoadCursorA(0, IDC_ARROW);
	wndclass->hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wndclass->lpszMenuName = 0;
	wndclass->lpszClassName = glib_application_name;

	if (::RegisterClassExA(wndclass) == 0) {
		return false;
	}
	return true;
}

static void glib_windows_unregister_window_class_ex(WNDCLASSEXA *wndclass) {
	::UnregisterClassA(glib_application_name, wndclass->hInstance);
}

WindowsPlatform::WindowsPlatform() : _IsValid(true) {
	if (!glib_windows_register_window_class_ex(&_WndClass)) {
		_IsValid = false;
		return;
	}
}

WindowsPlatform::~WindowsPlatform() {
	glib_windows_unregister_window_class_ex(&_WndClass);
}

bool WindowsPlatform::IsValid() const {
	return _IsValid;
}

/* -------------------------------------------------------------------- */
/** \name Window Managing
 * \{ */

WindowInterface *WindowsPlatform::InitWindow(WindowInterface *parent, int width, int height) {
	WindowsWindow *window = MEM_new<WindowsWindow>("glib::Window", reinterpret_cast<WindowsWindow *>(parent), width, height);
	if (window->GetContext()) {
		_Windows.push_back(window);
		return reinterpret_cast<WindowInterface *>(window);
	}
	MEM_delete<WindowsWindow>(reinterpret_cast<WindowsWindow *>(window));
	return nullptr;
}

void WindowsPlatform::CloseWindow(WindowInterface *window) {
	std::vector<WindowInterface *>::iterator itr = std::find(_Windows.begin(), _Windows.end(), window);
	if (itr != _Windows.end()) {
		_Windows.erase(itr);
		ClearWindowEvents(window);
		MEM_delete<WindowsWindow>(reinterpret_cast<WindowsWindow *>(window));
	}
	if (_Windows.empty()) {
		ClosePlatform();
	}
}

bool WindowsPlatform::IsWindow(WindowInterface *window) {
	std::vector<WindowInterface *>::iterator itr = std::find(_Windows.begin(), _Windows.end(), window);
	if (itr != _Windows.end()) {
		return true;
	}
	return false;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name System Managing
 * \{ */

bool WindowsPlatform::ProcessEvents(bool wait) {
	MSG msg;
	bool hasEventHandled = false;

	do {
		if (wait && !::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
			::Sleep(1);
		}

		/** Process all the events waiting for us. */
		while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
			/* #TranslateMessage doesn't alter the message, and doesn't change our raw keyboard data.
			 * Needed for #MapVirtualKey or if we ever need to get chars from wm_ime_char or similar. */
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
			hasEventHandled = true;
		}

		/* `PeekMessage` above is allowed to dispatch messages to the `wndproc` without us
		 * noticing, so we need to check the event manager here to see if there are
		 * events waiting in the queue. */
		hasEventHandled |= NumEvents() > 0;

	} while (wait && !hasEventHandled);

	return hasEventHandled;
}

/* \} */

WindowsWindow::WindowsWindow(WindowsWindow *parent, int width, int height) : _hWnd(NULL), _hDC(NULL), _Context(NULL) {
	DWORD ExtendedStyle = parent ? 0 : WS_EX_APPWINDOW;
	DWORD WindowStyle = parent ? WS_CHILD | WS_POPUP : WS_POPUP;
	HWND ParentHandle = (parent) ? reinterpret_cast<WindowsWindow *>(parent)->_hWnd : HWND_DESKTOP;

	int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	_hWnd = ::CreateWindowEx(ExtendedStyle,
							 glib_application_name,
							 "",
							 WindowStyle,
							 (ScreenWidth - width) / 2,
							 (ScreenHeight - height) / 2,
							 width,
							 height,
							 ParentHandle,
							 0,
							 ::GetModuleHandle(0),
							 0);

	if (_hWnd == NULL) {
		return;
	}

	_hDC = ::GetDC(_hWnd);

	const int TryInstall[] = {GLIB_CONTEXT_DIRECTX, GLIB_CONTEXT_OPENGL, GLIB_CONTEXT_VULKAN};

	for (size_t i = 0; i < sizeof(TryInstall) / sizeof(TryInstall[0]); i++) {
		if (InstallContext(static_cast<int>(i))) {
			break;
		}
	}

	if (!_Context) {
		return;
	}

	::ShowWindow(_hWnd, SW_SHOW);
	::UpdateWindow(_hWnd);

	::SetWindowLongPtr(_hWnd, GWLP_USERDATA, (LONG_PTR)this);
}

WindowsWindow::~WindowsWindow() {
	if (reinterpret_cast<WindowsWindow *>(::GetWindowLongPtr(_hWnd, GWLP_USERDATA)) == this) {
		::SetWindowLongPtr(_hWnd, GWLP_USERDATA, (LONG_PTR)NULL);

		::DestroyWindow(_hWnd);

		MEM_delete<ContextInterface>(_Context);
	}
}

/* -------------------------------------------------------------------- */
/** \name Size/Pos Managing
 * \{ */

void WindowsWindow::SetWindowSize(GSize size) {
	::SetWindowPos(_hWnd, NULL, 0, 0, size.x, size.y, SWP_NOZORDER | SWP_NOMOVE);
}

void WindowsWindow::SetClientSize(GSize size) {
	RECT window, client;
	::GetWindowRect(_hWnd, &window);
	::GetClientRect(_hWnd, &client);
	int dx = (window.right - window.left) - (client.right - client.left);
	int dy = (window.bottom - window.top) - (client.bottom - client.top);
	::SetWindowPos(_hWnd, NULL, 0, 0, size.x + dx, size.y + dy, SWP_NOZORDER | SWP_NOMOVE);
}

void WindowsWindow::SetPos(GPosition position) {
	::SetWindowPos(_hWnd, NULL, position.x, position.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void WindowsWindow::GetWindowRect(GRect *r_rect) const {
	RECT rect;
	::GetWindowRect(_hWnd, &rect);

	r_rect->left = rect.left;
	r_rect->top = rect.top;
	r_rect->right = rect.right;
	r_rect->bottom = rect.bottom;
}

void WindowsWindow::GetClientRect(GRect *r_rect) const {
	RECT rect;
	::GetClientRect(_hWnd, &rect);

	r_rect->left = rect.left;
	r_rect->top = rect.top;
	r_rect->right = rect.right;
	r_rect->bottom = rect.bottom;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Context Managing
 * \{ */

ContextInterface *WindowsWindow::InstallContext(int type) {
	switch (type) {
		case GLIB_CONTEXT_OPENGL: {
			WindowsOpenGLContext *context = MEM_new<WindowsOpenGLContext>("glib::WindowsOpenGLContext", this);
			if (context) {
				return _Context = context;
			}
		} break;
	}
	return NULL;
}

ContextInterface *WindowsWindow::GetContext() const {
	return _Context;
}

/* \} */

WindowsOpenGLContext::WindowsOpenGLContext(WindowsWindow *window) : _hDC(window->_hDC) {
	/**
	 * Each type I copy paste this thing I feel more and more guilty about having this hardcoded in my code.
	 */
	PIXELFORMATDESCRIPTOR PixelFormatDescriptor;
	memset(&PixelFormatDescriptor, 0, sizeof(PIXELFORMATDESCRIPTOR));

	PixelFormatDescriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	PixelFormatDescriptor.nVersion = 1;
	PixelFormatDescriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	PixelFormatDescriptor.iPixelType = PFD_TYPE_RGBA;
	PixelFormatDescriptor.cColorBits = 32;
	PixelFormatDescriptor.cDepthBits = 24;
	PixelFormatDescriptor.cStencilBits = 8;
	PixelFormatDescriptor.cAuxBuffers = 0;
	PixelFormatDescriptor.iLayerType = PFD_MAIN_PLANE;

	int PixelFormat = ::ChoosePixelFormat(_hDC, &PixelFormatDescriptor);
	::SetPixelFormat(_hDC, PixelFormat, &PixelFormatDescriptor);

	_hGLRC = ::wglCreateContext(_hDC);
	if (!_hGLRC) {
		return;
	}

	if (::wglMakeCurrent(_hDC, _hGLRC)) {
		GLenum err = glewInit();
		if (GLEW_OK != err) {
			/** Problem: glewInit failed, something is seriously wrong. */
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		}
		fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

		if (glewGetExtension("WGL_EXT_swap_control")) {
			::wglSwapIntervalEXT(0);
		}
	}
}

WindowsOpenGLContext::~WindowsOpenGLContext() {
	::wglDeleteContext(_hGLRC);
}

bool WindowsOpenGLContext::IsValid() {
	return _hGLRC;
}

/* -------------------------------------------------------------------- */
/** \name Context Management
 * \{ */

bool WindowsOpenGLContext::Activate() {
	if (::wglMakeCurrent(_hDC, _hGLRC)) {
		return true;
	}
	return false;
}

bool WindowsOpenGLContext::Deactivate() {
	if (::wglMakeCurrent(_hDC, NULL)) {
		return true;
	}
	return false;
}

bool WindowsOpenGLContext::SwapBuffers() {
	if (::SwapBuffers(_hDC)) {
		return true;
	}
	return false;
}

/* \} */

static int glib_windows_translate_mouse_modifiers(WPARAM wParam) {
	int ret = 0;

	ret |= (wParam & MK_CONTROL) ? GLIB_MODIFIER_CONTROL : 0;
	ret |= (wParam & MK_LBUTTON) ? GLIB_MODIFIER_LBTN : 0;
	ret |= (wParam & MK_MBUTTON) ? GLIB_MODIFIER_MBTN : 0;
	ret |= (wParam & MK_RBUTTON) ? GLIB_MODIFIER_RBTN : 0;
	ret |= (wParam & MK_SHIFT) ? GLIB_MODIFIER_SHIFT : 0;
	ret |= (wParam & MK_XBUTTON1) ? GLIB_MODIFIER_XBTN1 : 0;
	ret |= (wParam & MK_XBUTTON2) ? GLIB_MODIFIER_XBTN2 : 0;

	return ret;
}

LRESULT WindowsPlatform::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	bool handled = false;
	LRESULT result = 0;

	WindowsWindow *wnd = reinterpret_cast<WindowsWindow *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message) {
		case WM_DESTROY: {
			if (wnd) {
				glib_windows_platform->CloseWindow(wnd);
			}
			handled |= true;
		} break;
		case WM_NCHITTEST: {
			result = DefWindowProcA(hWnd, message, wParam, lParam);

			const int padding = 6;

			if (result == HTCLIENT) {
				POINT cursor;
				cursor.x = GET_X_LPARAM(lParam);
				cursor.y = GET_Y_LPARAM(lParam);

				RECT windowRect;
				GetWindowRect(hWnd, &windowRect);

				bool horizontal = cursor.x < windowRect.left + padding || cursor.x >= windowRect.right - padding;
				bool vertical = cursor.y < windowRect.top + padding || cursor.y >= windowRect.bottom - padding;
				if (horizontal && vertical) {
					LRESULT left = static_cast<LRESULT>(cursor.x < windowRect.left + padding);
					LRESULT right = static_cast<LRESULT>(cursor.x >= windowRect.right - padding);

					result = 0;
					result |= left * ((cursor.y < windowRect.top + padding) ? HTTOPLEFT : HTBOTTOMLEFT);
					result |= right * ((cursor.y < windowRect.top + padding) ? HTTOPRIGHT : HTBOTTOMRIGHT);
				}
				else if (horizontal || vertical) {
					result = 0;
					result |= (cursor.x < windowRect.left + padding) ? HTLEFT : 0;
					result |= (cursor.x >= windowRect.right - padding) ? HTRIGHT : 0;
					result |= (cursor.y < windowRect.top + padding) ? HTTOP : 0;
					result |= (cursor.y >= windowRect.bottom + padding) ? HTBOTTOM : 0;
				}
			}

			handled |= true;
		} break;
		case WM_GETMINMAXINFO: {
			LPMINMAXINFO info = (LPMINMAXINFO)lParam;
			
			info->ptMinTrackSize.x = 1024;
			info->ptMinTrackSize.y = (info->ptMinTrackSize.x * 9) >> 4;
			
			handled |= true;
		} break;
		case WM_SIZE: {
			int width = static_cast<int>(LOWORD(lParam));
			int height = static_cast<int>(HIWORD(lParam));

			glib_windows_platform->PostWindowSizeEvent(reinterpret_cast<WindowInterface *>(wnd), width, height);
			glib_windows_platform->DispatchEvents();

			handled |= true;
		} break;
		case WM_MOVE: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));

			glib_windows_platform->PostWindowMoveEvent(reinterpret_cast<WindowInterface *>(wnd), x, y);
			glib_windows_platform->DispatchEvents();

			handled |= true;
		} break;
		case WM_MOUSEMOVE: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_translate_mouse_modifiers(wParam);

			glib_windows_platform->PostMouseMoveEvent(reinterpret_cast<WindowInterface *>(wnd), x, y, modifiers);

			handled |= true;
		} break;
		case WM_MOUSEWHEEL: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_translate_mouse_modifiers(LOWORD(wParam));
			int delta = static_cast<short>(HIWORD(wParam));

			glib_windows_platform->PostMouseWheelEvent(reinterpret_cast<WindowInterface *>(wnd), x, y, modifiers, delta);

			handled |= true;
		} break;
		case WM_LBUTTONDOWN: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_translate_mouse_modifiers(wParam);

			glib_windows_platform->PostMouseButtonEvent(
				reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_LMOUSEDOWN, x, y, modifiers);

			handled |= true;
		} break;
		case WM_MBUTTONDOWN: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_translate_mouse_modifiers(wParam);

			glib_windows_platform->PostMouseButtonEvent(
				reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_MMOUSEDOWN, x, y, modifiers);

			handled |= true;
		} break;
		case WM_RBUTTONDOWN: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_translate_mouse_modifiers(wParam);

			glib_windows_platform->PostMouseButtonEvent(
				reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_RMOUSEDOWN, x, y, modifiers);

			handled |= true;
		} break;
		case WM_LBUTTONUP: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_translate_mouse_modifiers(wParam);

			glib_windows_platform->PostMouseButtonEvent(
				reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_LMOUSEUP, x, y, modifiers);

			handled |= true;
		} break;
		case WM_MBUTTONUP: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_translate_mouse_modifiers(wParam);

			glib_windows_platform->PostMouseButtonEvent(
				reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_MMOUSEUP, x, y, modifiers);

			handled |= true;
		} break;
		case WM_RBUTTONUP: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_translate_mouse_modifiers(wParam);

			glib_windows_platform->PostMouseButtonEvent(
				reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_RMOUSEUP, x, y, modifiers);

			handled |= true;
		} break;
	}

	if (handled) {
		return result;
	}
	return DefWindowProcA(hWnd, message, wParam, lParam);
}

/* -------------------------------------------------------------------- */
/** \name Public Platform Functions
 * \{ */

int InitPlatform(void) {
	if ((glib_windows_platform = MEM_new<WindowsPlatform>("glib::WindowPlatform")) == nullptr) {
		return 0xf00;
	}

	if (!glib_windows_platform->IsValid()) {
		ClosePlatform();
		return 0xf01;
	}

	glib_platform = reinterpret_cast<PlatformInterface *>(glib_windows_platform);

	return 0;
}

void ClosePlatform() {
	if (glib_windows_platform) {
		MEM_delete<WindowsPlatform>(glib_windows_platform);
		glib_windows_platform = nullptr;
		glib_platform = nullptr;
	}
}

/* \} */
