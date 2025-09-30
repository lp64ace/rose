#include "gtk_win32_window_manager.hh"

#include <windows.h>

GTKManagerWin32::GTKManagerWin32() {
	if (!this->InitKeyboardLayout()) {
		fprintf(stderr, "[GTK] Keyboard Layout initialization failed.\n");
	}
	if (!this->InitDummy()) {
		fprintf(stderr, "[GTK] Dummy window initialization failed.\n");
	}
	if (!this->InitExtensions()) {
		fprintf(stderr, "[GTK] WGL Extensions initialization failed.\n");
	}
}

GTKManagerWin32::~GTKManagerWin32() {
	this->ExitExtensions();
	this->ExitDummy();
	this->ExitKeyboardLayout();

	for (FormatSetting *format : this->formats) {
		MEM_delete<FormatSetting>(format);
	}
	this->formats.clear();
}

bool GTKManagerWin32::HasEvents() {
	MSG msg;
	if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
		return true;
	}
	return false;
}

void GTKManagerWin32::Poll() {
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

bool GTKManagerWin32::SetClipboard(const char *buffer, unsigned int length, bool selection) {
	if (selection || !buffer) {
		return false;
	}

	if (OpenClipboard(NULL)) {
		EmptyClipboard();

		INT ret = ::MultiByteToWideChar(CP_UTF8, 0, buffer, length, NULL, 0);

		HGLOBAL clipbuffer = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * (ret + 1));
		if (clipbuffer) {
			wchar_t *data = (wchar_t *)GlobalLock(clipbuffer);

			if (::MultiByteToWideChar(CP_UTF8, 0, buffer, length, data, ret + 1) < 0) {
				data[0] = '\0';
			}

			GlobalUnlock(clipbuffer);
			SetClipboardData(CF_UNICODETEXT, clipbuffer);
		}

		CloseClipboard();
	}

	return true;
}

bool GTKManagerWin32::GetClipboard(char **r_buffer, unsigned int *r_length, bool selection) const {
	if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL)) {
		wchar_t *buffer;
		HANDLE handle = GetClipboardData(CF_UNICODETEXT);
		if (!handle) {
			CloseClipboard();
			return false;
		}
		buffer = (wchar_t *)GlobalLock(handle);
		if (!buffer) {
			CloseClipboard();
			return false;
		}

		INT ret = ::WideCharToMultiByte(CP_UTF8, 0, buffer, -1, NULL, 0, NULL, NULL);

		*r_buffer = static_cast<char *>(MEM_mallocN(ret + 1, "Win32::Clipboard"));
		*r_length = static_cast<unsigned int>(ret);

		if (::WideCharToMultiByte(CP_UTF8, 0, buffer, ret, *r_buffer, *r_length, NULL, NULL) < 0) {
			r_buffer[0] = '\0';
		}

		*r_length = strlen(*r_buffer);

		GlobalUnlock(handle);
		CloseClipboard();

		return true;
	}

	return false;
}

GTKWindowWin32 *GTKManagerWin32::GetWindowByHandle(HWND hwnd) {
	for (auto window : this->GetWindows()) {
		if (static_cast<GTKWindowWin32 *>(window)->GetHandle() == hwnd) {
			return static_cast<GTKWindowWin32 *>(window);
		}
	}
	return NULL;
}

// http://msdn.microsoft.com/en-us/windows/hardware/gg487473.aspx
bool GTKManagerWin32::InitKeyboardLayout() {
	RAWINPUTDEVICE device;
	ZeroMemory(&device, sizeof(RAWINPUTDEVICE));
	device.usUsagePage = 0x01;
	device.usUsage = 0x06;
	if (RegisterRawInputDevices(&device, 1, sizeof(RAWINPUTDEVICE))) {
		this->keyboard_layout = ::GetKeyboardLayout(0);
		return true;
	}
	return false;
}

int GTKManagerWin32::GetLegacyPFD(const FormatSetting *desired, HDC hdc) {
	int nNative = 0;
	int nCompatible = 0;

	nNative = DescribePixelFormat(hdc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);

	for (FormatSetting *format : this->formats) {
		MEM_delete<FormatSetting>(format);
	}
	this->formats.clear();

	for (int iNative = 1; iNative <= nNative; iNative++) {
		PIXELFORMATDESCRIPTOR pfd;
		if (!DescribePixelFormat(hdc, iNative, sizeof(PIXELFORMATDESCRIPTOR), &pfd)) {
			continue;
		}

		// skip if the PFD does not have PFD_DRAW_TO_WINDOW and PFD_SUPPORT_OPENGL
		if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW) || !(pfd.dwFlags & PFD_SUPPORT_OPENGL)) {
			continue;
		}
		if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) && (pfd.dwFlags & PFD_GENERIC_FORMAT)) {
			continue;
		}
		// if the pixel type is not RGBA
		if (pfd.iPixelType != PFD_TYPE_RGBA) {
			continue;
		}

		FormatSetting *format = MEM_new<FormatSetting>("FormatSetting");
		{
			format->rbits = pfd.cRedBits, format->gbits = pfd.cGreenBits, format->bbits = pfd.cBlueBits;
			format->arbits = pfd.cAccumRedBits, format->agbits = pfd.cAccumGreenBits, format->abbits = pfd.cAccumBlueBits;
			format->dbits = pfd.cDepthBits, format->sbits = pfd.cStencilBits;
			format->abits = pfd.cAlphaBits;
			format->aabits = pfd.cAccumAlphaBits;

			format->aux = pfd.cAuxBuffers;

			format->has_stereo = (pfd.dwFlags & PFD_STEREO);
			format->has_double_buffer = (pfd.dwFlags & PFD_DOUBLEBUFFER);
		}
		format->handle = iNative;

		this->formats.push_back(format);

		nCompatible++;
	}

	FormatSetting *format = GetClosestFormat(this->formats, desired);

	if (format) {
		return format->handle;
	}

	return 0;
}

bool GTKManagerWin32::InitDummy() {
	ZeroMemory(&this->render_window_dummy.windowclass, sizeof(WNDCLASS));
	this->render_window_dummy.instance = GetModuleHandle(NULL);
	this->render_window_dummy.windowclass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	this->render_window_dummy.windowclass.cbClsExtra = 0;
	this->render_window_dummy.windowclass.cbWndExtra = 0;
	this->render_window_dummy.windowclass.lpfnWndProc = DefWindowProc;
	this->render_window_dummy.windowclass.hInstance = GetModuleHandle(NULL);
	this->render_window_dummy.windowclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	this->render_window_dummy.windowclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	this->render_window_dummy.windowclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	this->render_window_dummy.windowclass.lpszMenuName = NULL;
	this->render_window_dummy.windowclass.lpszClassName = "RenderWindowDummy";
	RegisterClass(&this->render_window_dummy.windowclass);

	this->render_window_dummy.window = CreateWindowEx(
		WS_EX_APPWINDOW,
		"RenderWindowDummy",
		"RenderWindowDummy",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1,
		1,
		HWND_DESKTOP,
		NULL,
		this->render_window_dummy.instance,
		NULL
	);

	if (!this->render_window_dummy.window) {
		return false;
	}

	ShowWindow(this->render_window_dummy.window, SW_HIDE);

	this->render_window_dummy.device = GetDC(this->render_window_dummy.window);

	FormatSetting format = {};
	int handle = GetLegacyPFD(&format, this->render_window_dummy.device);

	PIXELFORMATDESCRIPTOR pfd;
	if (!DescribePixelFormat(this->render_window_dummy.device, handle, sizeof(PIXELFORMATDESCRIPTOR), &pfd)) {
		return false;
	}
	if (!SetPixelFormat(this->render_window_dummy.device, handle, &pfd)) {
		return false;
	}

	this->render_window_dummy.render = wglCreateContext(this->render_window_dummy.device);
	if (!this->render_window_dummy.render) {
		return false;
	}
	if (!wglMakeCurrent(this->render_window_dummy.device, this->render_window_dummy.render)) {
		return false;
	}

	return true;
}

bool GTKManagerWin32::InitExtensions() {
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
	if (wglGetExtensionsStringARB == nullptr && wglGetExtensionsStringEXT == nullptr) {
		return false;
	}
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglChoosePixelFormatEXT = (PFNWGLCHOOSEPIXELFORMATEXTPROC)wglGetProcAddress("wglChoosePixelFormatEXT");
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

	wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
	wglGetPixelFormatAttribfvEXT = (PFNWGLGETPIXELFORMATATTRIBFVEXTPROC)wglGetProcAddress("wglGetPixelFormatAttribfvEXT");
	wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
	wglGetPixelFormatAttribivEXT = (PFNWGLGETPIXELFORMATATTRIBIVEXTPROC)wglGetProcAddress("wglGetPixelFormatAttribivEXT");
	return true;
}

void GTKManagerWin32::ExitExtensions() {
}

void GTKManagerWin32::ExitDummy() {
	if (this->render_window_dummy.render) {
		::wglDeleteContext(this->render_window_dummy.render);
	}
	if (this->render_window_dummy.device) {
		::ReleaseDC(this->render_window_dummy.window, this->render_window_dummy.device);
	}
	if (this->render_window_dummy.window) {
		::DestroyWindow(this->render_window_dummy.window);
	}
	UnregisterClass("RenderWindowDummy", this->render_window_dummy.instance);
}

void GTKManagerWin32::ExitKeyboardLayout() {
}

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

int ConvertInputKey(short win32_key, short scan_code, short extend) {
	int key = GTK_KEY_UNKOWN;

	if ((win32_key >= '0') && (win32_key <= '9')) {
		key = (win32_key - '0' + GTK_KEY_0);
	}
	else if ((win32_key >= 'A') && (win32_key <= 'Z')) {
		key = (win32_key - 'A' + GTK_KEY_A);
	}
	else if ((win32_key >= VK_F1) && (win32_key <= VK_F24)) {
		key = (win32_key - VK_F1 + GTK_KEY_F1);
	}
	else {
		switch (win32_key) {
			case VK_RETURN: {
				key = (extend) ? GTK_KEY_NUMPAD_ENTER : GTK_KEY_ENTER;
			} break;
			case VK_SPACE: {
				key = GTK_KEY_SPACE;
			} break;
			case VK_BACK: {
				key = GTK_KEY_BACKSPACE;
			} break;
			case VK_TAB: {
				key = GTK_KEY_TAB;
			} break;
			case VK_ESCAPE: {
				key = GTK_KEY_ESC;
			} break;
			case VK_INSERT:
			case VK_NUMPAD0: {
				key = (extend) ? GTK_KEY_INSERT : GTK_KEY_NUMPAD_0;
			} break;
			case VK_END:
			case VK_NUMPAD1: {
				key = (extend) ? GTK_KEY_END : GTK_KEY_NUMPAD_1;
			} break;
			case VK_DOWN:
			case VK_NUMPAD2: {
				key = (extend) ? GTK_KEY_DOWN : GTK_KEY_NUMPAD_2;
			} break;
			case VK_NEXT:
			case VK_NUMPAD3: {
				key = (extend) ? GTK_KEY_PAGEDOWN : GTK_KEY_NUMPAD_3;
			} break;
			case VK_LEFT:
			case VK_NUMPAD4: {
				key = (extend) ? GTK_KEY_LEFT : GTK_KEY_NUMPAD_4;
			} break;
			case VK_CLEAR:
			case VK_NUMPAD5: {
				key = (extend) ? GTK_KEY_UNKOWN : GTK_KEY_NUMPAD_5;
			} break;
			case VK_RIGHT:
			case VK_NUMPAD6: {
				key = (extend) ? GTK_KEY_RIGHT : GTK_KEY_NUMPAD_6;
			} break;
			case VK_HOME:
			case VK_NUMPAD7: {
				key = (extend) ? GTK_KEY_HOME : GTK_KEY_NUMPAD_7;
			} break;
			case VK_UP:
			case VK_NUMPAD8: {
				key = (extend) ? GTK_KEY_UP : GTK_KEY_NUMPAD_8;
			} break;
			case VK_PRIOR:
			case VK_NUMPAD9: {
				key = (extend) ? GTK_KEY_PAGEUP : GTK_KEY_NUMPAD_9;
			} break;
			case VK_DECIMAL:
			case VK_DELETE: {
				key = (extend) ? GTK_KEY_DELETE : GTK_KEY_NUMPAD_PERIOD;
			} break;

			case VK_SNAPSHOT: {
				key = GTK_KEY_PRTSCN;
			} break;
			case VK_PAUSE: {
				key = GTK_KEY_PAUSE;
			} break;
			case VK_MULTIPLY: {
				key = GTK_KEY_NUMPAD_ASTERISK;
			} break;
			case VK_SUBTRACT: {
				key = GTK_KEY_NUMPAD_MINUS;
			} break;
			case VK_DIVIDE: {
				key = GTK_KEY_NUMPAD_SLASH;
			} break;
			case VK_ADD: {
				key = GTK_KEY_NUMPAD_PLUS;
			} break;

			case VK_SEMICOLON: {
				key = GTK_KEY_SEMICOLON;
			} break;
			case VK_EQUALS: {
				key = GTK_KEY_EQUAL;
			} break;
			case VK_COMMA: {
				key = GTK_KEY_COMMA;
			} break;
			case VK_MINUS: {
				key = GTK_KEY_MINUS;
			} break;
			case VK_PERIOD: {
				key = GTK_KEY_PERIOD;
			} break;
			case VK_SLASH: {
				key = GTK_KEY_SLASH;
			} break;
			case VK_BACK_QUOTE: {
				key = GTK_KEY_ACCENTGRAVE;
			} break;
			case VK_OPEN_BRACKET: {
				key = GTK_KEY_LEFT_BRACKET;
			} break;
			case VK_BACK_SLASH: {
				key = GTK_KEY_BACKSLASH;
			} break;
			case VK_CLOSE_BRACKET: {
				key = GTK_KEY_RIGHT_BRACKET;
			} break;
			case VK_GR_LESS: {
				key = GTK_KEY_GRLESS;
			} break;

			case VK_SHIFT: {
				if (scan_code == 0x36) {
					key = GTK_KEY_RIGHT_SHIFT;
				}
				else if (scan_code == 0x2a) {
					key = GTK_KEY_LEFT_SHIFT;
				}
				else {
					key = GTK_KEY_UNKOWN;
				}
			} break;
			case VK_CONTROL: {
				key = (extend) ? GTK_KEY_RIGHT_CTRL : GTK_KEY_LEFT_CTRL;
			} break;
			case VK_MENU: {
				key = (extend) ? GTK_KEY_RIGHT_ALT : GTK_KEY_LEFT_ALT;
			} break;
			case VK_LWIN: {
				key = GTK_KEY_LEFT_OS;
			} break;
			case VK_RWIN: {
				key = GTK_KEY_RIGHT_OS;
			} break;
			case VK_APPS: {
				key = GTK_KEY_APP;
			} break;
			case VK_NUMLOCK: {
				key = GTK_KEY_NUMLOCK;
			} break;
			case VK_SCROLL: {
				key = GTK_KEY_SCRLOCK;
			} break;
			case VK_CAPITAL: {
				key = GTK_KEY_CAPSLOCK;
			} break;
			case VK_MEDIA_PLAY_PAUSE: {
				key = GTK_KEY_MEDIA_PLAY;
			} break;
			case VK_MEDIA_STOP: {
				key = GTK_KEY_MEDIA_STOP;
			} break;
			case VK_MEDIA_PREV_TRACK: {
				key = GTK_KEY_MEDIA_FIRST;
			} break;
			case VK_MEDIA_NEXT_TRACK: {
				key = GTK_KEY_MEDIA_LAST;
			} break;
		}
	}

	return key;
}
