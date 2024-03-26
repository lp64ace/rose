#include <gl/glew.h>
#include <gl/wglew.h>
#include <windowsx.h>

#include <algorithm>

#include "MEM_alloc.h"

#include "platform_win32.hh"

#ifndef VK_MINUS
#	define VK_MINUS 0xBD
#endif /* VK_MINUS */
#ifndef VK_SEMICOLON
#	define VK_SEMICOLON 0xBA
#endif /* VK_SEMICOLON */
#ifndef VK_PERIOD
#	define VK_PERIOD 0xBE
#endif /* VK_PERIOD */
#ifndef VK_COMMA
#	define VK_COMMA 0xBC
#endif /* VK_COMMA */
#ifndef VK_BACK_QUOTE
#	define VK_BACK_QUOTE 0xC0
#endif /* VK_BACK_QUOTE */
#ifndef VK_SLASH
#	define VK_SLASH 0xBF
#endif /* VK_SLASH */
#ifndef VK_BACK_SLASH
#	define VK_BACK_SLASH 0xDC
#endif /* VK_BACK_SLASH */
#ifndef VK_EQUALS
#	define VK_EQUALS 0xBB
#endif /* VK_EQUALS */
#ifndef VK_OPEN_BRACKET
#	define VK_OPEN_BRACKET 0xDB
#endif /* VK_OPEN_BRACKET */
#ifndef VK_CLOSE_BRACKET
#	define VK_CLOSE_BRACKET 0xDD
#endif /* VK_CLOSE_BRACKET */
#ifndef VK_GR_LESS
#	define VK_GR_LESS 0xE2
#endif /* VK_GR_LESS */

static class WindowsPlatform *glib_windows_platform = nullptr;

/* -------------------------------------------------------------------- */
/** \name Internal Utils
 * \{ */

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

static int glib_windows_modifiers_get(void) {
	int ret;

	ret |= (HIBYTE(::GetAsyncKeyState(VK_LSHIFT)) != 0) ? GLIB_MODIFIER_LSHIFT : 0;
	ret |= (HIBYTE(::GetAsyncKeyState(VK_RSHIFT)) != 0) ? GLIB_MODIFIER_RSHIFT : 0;

	ret |= (HIBYTE(::GetAsyncKeyState(VK_LMENU)) != 0) ? GLIB_MODIFIER_LALT : 0;
	ret |= (HIBYTE(::GetAsyncKeyState(VK_RMENU)) != 0) ? GLIB_MODIFIER_RALT : 0;

	ret |= (HIBYTE(::GetAsyncKeyState(VK_LCONTROL)) != 0) ? GLIB_MODIFIER_LCONTROL : 0;
	ret |= (HIBYTE(::GetAsyncKeyState(VK_RCONTROL)) != 0) ? GLIB_MODIFIER_RCONTROL : 0;

	ret |= (HIBYTE(::GetAsyncKeyState(VK_LWIN)) != 0) ? GLIB_MODIFIER_LOS : 0;
	ret |= (HIBYTE(::GetAsyncKeyState(VK_RWIN)) != 0) ? GLIB_MODIFIER_ROS : 0;

	return ret;
}

static bool glib_windows_init_raw_input(HKL *layout) {
	RAWINPUTDEVICE device;
	memset(&device, 0, sizeof(RAWINPUTDEVICE));

	/* http://msdn.microsoft.com/en-us/windows/hardware/gg487473.aspx */
	device.usUsagePage = 0x01;
	device.usUsage = 0x06;

	if (RegisterRawInputDevices(&device, 1, sizeof(RAWINPUTDEVICE))) {
		*layout = ::GetKeyboardLayout(0);
		return true;
	}
	return false;
}

/* \} */

WindowsPlatform::WindowsPlatform() : _IsValid(true) {
	if (!glib_windows_register_window_class_ex(&_WndClass)) {
		_IsValid = false;
		return;
	}

	if (!glib_windows_init_raw_input(&_KeyboardLayout)) {
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
/** \name Generic Utils
 * \{ */

/** Returns the size of the main display. */
GSize WindowsPlatform::GetScreenSize() const {
	int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	GSize result;

	result.x = ScreenWidth;
	result.y = ScreenHeight;

	return result;
}

GPosition WindowsPlatform::GetCursorPosition(WindowInterface *window) const {
	POINT pt;
	if (::GetCursorPos(&pt) && window != NULL) {
		::ScreenToClient(reinterpret_cast<WindowsWindow *>(window)->AsHandle(), &pt);
	}
	return {pt.x, pt.y};
}

WindowInterface *WindowsPlatform::GetWindowUnderCursor(int x, int y) const {
	POINT point;
	if (!GetCursorPos(&point)) {
		return NULL;
	}

	HWND win = ::WindowFromPoint(point);
	if (win == NULL) {
		return NULL;
	}

	WindowsWindow *wnd = reinterpret_cast<WindowsWindow *>(::GetWindowLongPtr(win, GWLP_USERDATA));
	/** Other windows are allowed to have userdata... */
	if (std::find(_Windows.begin(), _Windows.end(), wnd) != _Windows.end()) {
		return wnd;
	}
	return NULL;
}

/* \} */

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

		/** This event needs to be handled now since the window will not be valid any more after this... */
		PostDestroyEvent(window);

		MEM_delete<WindowsWindow>(reinterpret_cast<WindowsWindow *>(window));
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

WindowsWindow::WindowsWindow(WindowsWindow *parent, int width, int height) : _hWnd(NULL), _hDC(NULL), _Context(NULL), _ContextType(GLIB_CONTEXT_NONE) {
	DWORD ExtendedStyle = parent ? 0 : WS_EX_APPWINDOW;
#if 1
	/**
	 * If it works it will look magnificent, but due to the window moving inside a handler we have issues with maintaining
	 * consistency accross mouse position in events so for now we fallback to the default windows behaviour where the movement
	 * of the window is handled by #DefWindowProc.
	 *
	 * p.s. I tried to make it work for like 4h now but I can't seem to be competent right now... Maybe I will give it a try
	 * again another day, that way I get to keep my setup intact :)
	 */
	DWORD WindowStyle = parent ? WS_CHILD | WS_POPUP : WS_POPUP;
#else
	DWORD WindowStyle = parent ? WS_CHILD | WS_OVERLAPPEDWINDOW : WS_OVERLAPPEDWINDOW;
#endif
	HWND ParentHandle = (parent) ? reinterpret_cast<WindowsWindow *>(parent)->_hWnd : HWND_DESKTOP;

	int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	_hWnd = ::CreateWindowEx(ExtendedStyle, glib_application_name, "Rose", WindowStyle, (ScreenWidth - width) / 2, (ScreenHeight - height) / 2, width, height, ParentHandle, 0, ::GetModuleHandle(0), 0);

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
	}

	MEM_delete<ContextInterface>(_Context);
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

bool WindowsWindow::IsIconic() const {
	if (::IsIconic(_hWnd)) {
		return true;
	}
	return false;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Window Utils
 * \{ */

GPosition WindowsWindow::ScreenToClient(int x, int y) const {
	POINT pt = {x, y};
	::ScreenToClient(_hWnd, &pt);
	return {pt.x, pt.y};
}

GPosition WindowsWindow::ClientToScreen(int x, int y) const {
	POINT pt = {x, y};
	::ClientToScreen(_hWnd, &pt);
	return {pt.x, pt.y};
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Context Managing
 * \{ */

ContextInterface *WindowsWindow::InstallContext(int type) {
	if (type == _ContextType && _Context != NULL) {
		return _Context;
	}

	switch (type) {
		case GLIB_CONTEXT_OPENGL: {
			WindowsOpenGLContext *context = MEM_new<WindowsOpenGLContext>("glib::WindowsOpenGLContext", this);
			if (context) {
				_ContextType = GLIB_CONTEXT_OPENGL;
				return _Context = context;
			}
		} break;
	}

	return NULL;
}

ContextInterface *WindowsWindow::GetContext() const {
	return _Context;
}

int WindowsWindow::GetContextType() const {
	return (_Context) ? _ContextType : GLIB_CONTEXT_NONE;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Local
 * \{ */

HWND WindowsWindow::AsHandle() const {
	return _hWnd;
}

/* \} */

static HGLRC s_SharedHGLRC = nullptr;
static int s_SharedCount = 0;

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

	if (s_SharedHGLRC == nullptr) {
		s_SharedHGLRC = _hGLRC;
	}
	else if (::wglShareLists(s_SharedHGLRC, _hGLRC)) {
	}

	s_SharedCount++;

	if (::wglMakeCurrent(_hDC, _hGLRC)) {
		GLenum err = glewInit();
		if (GLEW_OK != err) {
			/** Problem: glewInit failed, something is seriously wrong. */
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		}

		if (glewGetExtension("WGL_EXT_swap_control")) {
			::wglSwapIntervalEXT(0);
		}
	}
}

WindowsOpenGLContext::~WindowsOpenGLContext() {
	if (_hGLRC != nullptr) {
		if (_hGLRC == ::wglGetCurrentContext())
			::wglMakeCurrent(nullptr, nullptr);

		if (_hGLRC != s_SharedHGLRC || s_SharedCount == 1) {
			s_SharedCount--;

			if (s_SharedCount == 0) {
				s_SharedHGLRC = nullptr;
			}

			::wglDeleteContext(_hGLRC);
		}
	}
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

static int ConvertKey(short vKey, short scanCode, short extend) {
	int key = GLIB_KEY_UNKOWN;

	if ((vKey >= '0') && (vKey <= '9')) {
		key = (vKey - '0' + GLIB_KEY_0);
	}
	else if ((vKey >= 'A') && (vKey <= 'Z')) {
		key = (vKey - 'A' + GLIB_KEY_A);
	}
	else if ((vKey >= VK_F1) && (vKey <= VK_F24)) {
		key = (vKey - VK_F1 + GLIB_KEY_F1);
	}
	else {
		switch (key) {
			case VK_RETURN: {
				key = (extend) ? GLIB_KEY_NUMPAD_ENTER : GLIB_KEY_ENTER;
			} break;
			case VK_BACK: {
				key = GLIB_KEY_BACKSPACE;
			} break;
			case VK_TAB: {
				key = GLIB_KEY_TAB;
			} break;
			case VK_ESCAPE: {
				key = GLIB_KEY_ESC;
			} break;
			case VK_INSERT:
			case VK_NUMPAD0: {
				key = (extend) ? GLIB_KEY_INSERT : GLIB_KEY_NUMPAD_0;
			} break;
			case VK_END:
			case VK_NUMPAD1: {
				key = (extend) ? GLIB_KEY_END : GLIB_KEY_NUMPAD_1;
			} break;
			case VK_DOWN:
			case VK_NUMPAD2: {
				key = (extend) ? GLIB_KEY_DOWN : GLIB_KEY_NUMPAD_2;
			} break;
			case VK_NEXT:
			case VK_NUMPAD3: {
				key = (extend) ? GLIB_KEY_PAGEDOWN : GLIB_KEY_NUMPAD_3;
			} break;
			case VK_LEFT:
			case VK_NUMPAD4: {
				key = (extend) ? GLIB_KEY_LEFT : GLIB_KEY_NUMPAD_4;
			} break;
			case VK_CLEAR:
			case VK_NUMPAD5: {
				key = (extend) ? GLIB_KEY_UNKOWN : GLIB_KEY_NUMPAD_5;
			} break;
			case VK_RIGHT:
			case VK_NUMPAD6: {
				key = (extend) ? GLIB_KEY_RIGHT : GLIB_KEY_NUMPAD_6;
			} break;
			case VK_HOME:
			case VK_NUMPAD7: {
				key = (extend) ? GLIB_KEY_HOME : GLIB_KEY_NUMPAD_7;
			} break;
			case VK_UP:
			case VK_NUMPAD8: {
				key = (extend) ? GLIB_KEY_UP : GLIB_KEY_NUMPAD_8;
			} break;
			case VK_PRIOR:
			case VK_NUMPAD9: {
				key = (extend) ? GLIB_KEY_PAGEUP : GLIB_KEY_NUMPAD_9;
			} break;
			case VK_DECIMAL:
			case VK_DELETE: {
				key = (extend) ? GLIB_KEY_DELETE : GLIB_KEY_NUMPAD_PERIOD;
			} break;

			case VK_SNAPSHOT: {
				key = GLIB_KEY_PRTSCN;
			} break;
			case VK_PAUSE: {
				key = GLIB_KEY_PAUSE;
			} break;
			case VK_MULTIPLY: {
				key = GLIB_KEY_NUMPAD_ASTERISK;
			} break;
			case VK_SUBTRACT: {
				key = GLIB_KEY_NUMPAD_MINUS;
			} break;
			case VK_DIVIDE: {
				key = GLIB_KEY_NUMPAD_SLASH;
			} break;
			case VK_ADD: {
				key = GLIB_KEY_NUMPAD_PLUS;
			} break;

			case VK_SEMICOLON: {
				key = GLIB_KEY_SEMICOLON;
			} break;
			case VK_EQUALS: {
				key = GLIB_KEY_EQUAL;
			} break;
			case VK_COMMA: {
				key = GLIB_KEY_COMMA;
			} break;
			case VK_MINUS: {
				key = GLIB_KEY_MINUS;
			} break;
			case VK_PERIOD: {
				key = GLIB_KEY_PERIOD;
			} break;
			case VK_SLASH: {
				key = GLIB_KEY_SLASH;
			} break;
			case VK_BACK_QUOTE: {
				key = GLIB_KEY_ACCENTGRAVE;
			} break;
			case VK_OPEN_BRACKET: {
				key = GLIB_KEY_LEFT_BRACKET;
			} break;
			case VK_BACK_SLASH: {
				key = GLIB_KEY_BACKSLASH;
			} break;
			case VK_CLOSE_BRACKET: {
				key = GLIB_KEY_RIGHT_BRACKET;
			} break;
			case VK_GR_LESS: {
				key = GLIB_KEY_GRLESS;
			} break;

			case VK_SHIFT: {
				if (scanCode == 0x36) {
					key = GLIB_KEY_RIGHT_SHIFT;
				}
				else if (scanCode == 0x2a) {
					key = GLIB_KEY_LEFT_SHIFT;
				}
				else {
					key = GLIB_KEY_UNKOWN;
				}
			} break;
			case VK_CONTROL: {
				key = (extend) ? GLIB_KEY_RIGHT_CTRL : GLIB_KEY_LEFT_CTRL;
			} break;
			case VK_MENU: {
				key = (extend) ? GLIB_KEY_RIGHT_ALT : GLIB_KEY_LEFT_ALT;
			} break;
			case VK_LWIN: {
				key = GLIB_KEY_LEFT_OS;
			} break;
			case VK_RWIN: {
				key = GLIB_KEY_RIGHT_OS;
			} break;
			case VK_APPS: {
				key = GLIB_KEY_APP;
			} break;
			case VK_NUMLOCK: {
				key = GLIB_KEY_NUMLOCK;
			} break;
			case VK_SCROLL: {
				key = GLIB_KEY_SCRLOCK;
			} break;
			case VK_CAPITAL: {
				key = GLIB_KEY_CAPSLOCK;
			} break;
			case VK_MEDIA_PLAY_PAUSE: {
				key = GLIB_KEY_MEDIA_PLAY;
			} break;
			case VK_MEDIA_STOP: {
				key = GLIB_KEY_MEDIA_STOP;
			} break;
			case VK_MEDIA_PREV_TRACK: {
				key = GLIB_KEY_MEDIA_FIRST;
			} break;
			case VK_MEDIA_NEXT_TRACK: {
				key = GLIB_KEY_MEDIA_LAST;
			} break;
		}
	}

	return key;
}

static int HardKey(RAWINPUT *raw, bool *keydown) {
	int msg = raw->data.keyboard.Message;
	*keydown = !(raw->data.keyboard.Flags & RI_KEY_BREAK) && msg != WM_KEYUP && msg != WM_SYSKEYUP;

	return ConvertKey(raw->data.keyboard.VKey, raw->data.keyboard.MakeCode, (raw->data.keyboard.Flags & (RI_KEY_E1 | RI_KEY_E0)));
}

static bool ProcessKeyEvent(WindowsWindow *window, RAWINPUT *raw) {
	const USHORT vk = raw->data.keyboard.VKey;
	bool keydown = false;

	int key = HardKey(raw, &keydown);
	int modifiers = glib_windows_modifiers_get();

	bool repeat = false;
	bool repeat_modifier = false;

	if (keydown) {
		if (HIBYTE(::GetKeyState(vk)) != 0) {
			repeat = true;
			repeat_modifier = GLIB_KEY_MODIFIER_CHECK(key);
		}
	}

	if (!repeat_modifier) {
		wchar_t utf16[2] = {0};
		char utf8[4] = {0};
		BYTE state[256];

		const BOOL HasState = ::GetKeyboardState((PBYTE)state);
		const BOOL CtrlPressed = HasState && state[VK_CONTROL] & 0x80;
		const BOOL AltPressed = HasState && state[VK_MENU] & 0x80;

		/** No text with control key pressed (Alt can be used to insert special characters though!). */
		if (CtrlPressed && !AltPressed) {
		}
		else if (::MapVirtualKeyW(vk, MAPVK_VK_TO_CHAR) != 0) {
			INT Ret;

			if ((Ret = ::ToUnicodeEx(vk, raw->data.keyboard.MakeCode, state, utf16, 2, 0, glib_windows_platform->_KeyboardLayout))) {
				Ret = ::WideCharToMultiByte(CP_UTF8, 0, utf16, Ret, utf8, sizeof(utf8), NULL, NULL);
				if (0 <= Ret && Ret < sizeof(utf8) / sizeof(utf8[0])) {
					utf8[Ret] = '\0';
				}
			}
		}

		if (keydown) {
			glib_windows_platform->PostKeyDownEvent(window, key, repeat, modifiers, utf8);
		}
		else {
			glib_windows_platform->PostKeyUpEvent(window, key, repeat, modifiers, utf8);
		}

		return true;
	}

	return false;
}

LRESULT WindowsPlatform::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	bool handled = false;
	LRESULT result = 0;

	WindowsWindow *wnd = reinterpret_cast<WindowsWindow *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));

	switch (message) {
		case WM_DESTROY: {
			if (wnd) {
				::SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
				glib_windows_platform->CloseWindow(wnd);
			}
			handled |= true;
		} break;
		case WM_NCHITTEST: {
			result = DefWindowProcA(hWnd, message, wParam, lParam);

			const int rad = 6;

			POINT cursor;
			cursor.x = GET_X_LPARAM(lParam);
			cursor.y = GET_Y_LPARAM(lParam);

			RECT windowRect;
			GetWindowRect(hWnd, &windowRect);

			GRect caption = wnd->_CaptionRect;

			if (result == HTCLIENT) {
				bool horizontal = cursor.x < windowRect.left + rad || cursor.x >= windowRect.right - rad;
				bool vertical = cursor.y < windowRect.top + rad || cursor.y >= windowRect.bottom - rad;
				if (horizontal && vertical) {
					LRESULT left = static_cast<LRESULT>(cursor.x < windowRect.left + rad);
					LRESULT right = static_cast<LRESULT>(cursor.x >= windowRect.right - rad);

					result = 0;
					result |= left * ((cursor.y < windowRect.top + rad) ? HTTOPLEFT : HTBOTTOMLEFT);
					result |= right * ((cursor.y < windowRect.top + rad) ? HTTOPRIGHT : HTBOTTOMRIGHT);
				}
				else if (horizontal || vertical) {
					result = 0;
					result |= (cursor.x < windowRect.left + rad) ? HTLEFT : 0;
					result |= (cursor.x >= windowRect.right - rad) ? HTRIGHT : 0;
					result |= (cursor.y < windowRect.top + rad) ? HTTOP : 0;
					result |= (cursor.y >= windowRect.bottom + rad) ? HTBOTTOM : 0;
				}
			}
			if (result == HTCLIENT) {
				if (windowRect.left + caption.left <= cursor.x && cursor.x <= windowRect.left + caption.right) {
					if (windowRect.top + caption.top <= cursor.y && cursor.y <= windowRect.top + caption.bottom) {
						result = HTCAPTION;
					}
				}
			}

			handled |= true;
		} break;
		case WM_GETMINMAXINFO: {
			LPMINMAXINFO info = (LPMINMAXINFO)lParam;

			info->ptMinTrackSize.x = 800;
			info->ptMinTrackSize.y = (info->ptMinTrackSize.x * 9) >> 4;

			handled |= true;
		} break;
		case WM_SIZE: {
			int width = static_cast<int>(LOWORD(lParam));
			int height = static_cast<int>(HIWORD(lParam));

			if (wnd) {
				glib_windows_platform->PostWindowSizeEvent(reinterpret_cast<WindowInterface *>(wnd), width, height);
				glib_windows_platform->DispatchEvents();
			}

			handled |= true;
		} break;
		case WM_MOVE: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));

			if (wnd) {
				glib_windows_platform->PostWindowMoveEvent(reinterpret_cast<WindowInterface *>(wnd), x, y);
				glib_windows_platform->DispatchEvents();
			}

			handled |= true;
		} break;
		case WM_MOUSEMOVE: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_modifiers_get();

			glib_windows_platform->PostMouseMoveEvent(reinterpret_cast<WindowInterface *>(wnd), x, y, modifiers);

			TRACKMOUSEEVENT trackinfo;

			trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
			trackinfo.dwFlags = TME_LEAVE;
			trackinfo.hwndTrack = hWnd;
			trackinfo.dwHoverTime = HOVER_DEFAULT;

			::TrackMouseEvent(&trackinfo);

			handled |= true;
		} break;
		case WM_MOUSEWHEEL: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_modifiers_get();
			int delta = static_cast<short>(HIWORD(wParam));

			glib_windows_platform->PostMouseWheelEvent(reinterpret_cast<WindowInterface *>(wnd), x, y, modifiers, delta);

			handled |= true;
		} break;
		case WM_LBUTTONDOWN: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_modifiers_get();

			glib_windows_platform->PostMouseButtonEvent(reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_LMOUSEDOWN, x, y, modifiers);

			handled |= true;
		} break;
		case WM_MBUTTONDOWN: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_modifiers_get();

			glib_windows_platform->PostMouseButtonEvent(reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_MMOUSEDOWN, x, y, modifiers);

			handled |= true;
		} break;
		case WM_RBUTTONDOWN: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_modifiers_get();

			glib_windows_platform->PostMouseButtonEvent(reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_RMOUSEDOWN, x, y, modifiers);

			handled |= true;
		} break;
		case WM_LBUTTONUP: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_modifiers_get();

			glib_windows_platform->PostMouseButtonEvent(reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_LMOUSEUP, x, y, modifiers);

			handled |= true;
		} break;
		case WM_MBUTTONUP: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_modifiers_get();

			glib_windows_platform->PostMouseButtonEvent(reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_MMOUSEUP, x, y, modifiers);

			handled |= true;
		} break;
		case WM_RBUTTONUP: {
			int x = static_cast<int>(LOWORD(lParam));
			int y = static_cast<int>(HIWORD(lParam));
			int modifiers = glib_windows_modifiers_get();

			glib_windows_platform->PostMouseButtonEvent(reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_RMOUSEUP, x, y, modifiers);

			handled |= true;
		} break;
		case WM_MOUSELEAVE: {
			GPosition pos = glib_windows_platform->GetCursorPosition(reinterpret_cast<WindowInterface *>(wnd));
			int modifiers = glib_windows_modifiers_get();

			glib_windows_platform->PostMouseButtonEvent(reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_LMOUSEUP, pos.x, pos.y, modifiers);
			glib_windows_platform->PostMouseButtonEvent(reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_MMOUSEUP, pos.x, pos.y, modifiers);
			glib_windows_platform->PostMouseButtonEvent(reinterpret_cast<WindowInterface *>(wnd), GLIB_EVT_RMOUSEUP, pos.x, pos.y, modifiers);

			handled |= true;
		} break;
		case WM_INPUTLANGCHANGE: {
			glib_windows_platform->_KeyboardLayout = (HKL)lParam;
		} break;
		case WM_INPUT: {
			RAWINPUT Raw;
			UINT RawSize = sizeof(RAWINPUT);

			::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &Raw, &RawSize, sizeof(RAWINPUTHEADER));

			switch (Raw.header.dwType) {
				case RIM_TYPEKEYBOARD: {
					handled |= ProcessKeyEvent(wnd, &Raw);
				} break;
			}
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
	if (glib_windows_platform) {
		return 0;
	}

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
