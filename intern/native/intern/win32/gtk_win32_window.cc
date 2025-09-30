#include "gtk_win32_render.hh"
#include "gtk_win32_window.hh"
#include "gtk_win32_window_manager.hh"

#include "GTK_window_type.h"

#include <dwmapi.h>
#include <windowsx.h>

LRESULT CALLBACK WindowProcedure(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam);

GTKWindowWin32::GTKWindowWin32(GTKManagerWin32 *manager) : GTKWindowInterface(static_cast<GTKManagerInterface *>(manager)) {
	ZeroMemory(&this->windowclass, sizeof(WNDCLASS));
	this->hwnd = NULL;
	this->device = NULL;
	this->state = GTK_STATE_HIDDEN;
}

GTKWindowWin32::~GTKWindowWin32() {
	SetWindowLongPtr(this->hwnd, GWLP_USERDATA, (LONG_PTR)NULL);

	this->Install(GTK_WINDOW_RENDER_NONE);

	if (this->hwnd) {
		::DestroyWindow(this->hwnd);
	}
}

bool GTKWindowWin32::Create(GTKWindowInterface *vparent, const char *name, int width, int height, int state) {
	this->windowclass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	this->windowclass.cbClsExtra = 0;
	this->windowclass.cbWndExtra = 0;
	this->windowclass.lpfnWndProc = WindowProcedure;
	this->windowclass.hInstance = GetModuleHandle(NULL);
	this->windowclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	this->windowclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	this->windowclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	this->windowclass.lpszMenuName = NULL;
	this->windowclass.lpszClassName = name;
	RegisterClass(&this->windowclass);

	GTKWindowWin32 *parent = static_cast<GTKWindowWin32 *>(vparent);

	this->hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		name,
		name,
		WS_OVERLAPPEDWINDOW & ~WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		(parent) ? parent->hwnd : HWND_DESKTOP,
		NULL,
		GetModuleHandle(NULL),
		NULL
	);

	if (this->hwnd == NULL) {
		return false;
	}

	SetWindowLongPtr(this->hwnd, GWLP_USERDATA, (LONG_PTR)this->GetManagerInterface());

	this->device = GetDC(this->hwnd);

	if (this->device == NULL) {
		return false;
	}

	this->ThemeRefresh();

	switch (state) {
		case GTK_STATE_MINIMIZED: {
			this->Minimize();
		} break;
		case GTK_STATE_MAXIMIZED: {
			this->Maximize();
		} break;
		case GTK_STATE_RESTORED: {
			this->Show();
		} break;
		case GTK_STATE_HIDDEN: {
			this->Hide();
		} break;
	}

	return true;
}

void GTKWindowWin32::Minimize() {
	ShowWindow(this->hwnd, SW_MINIMIZE);
}
void GTKWindowWin32::Maximize() {
	ShowWindow(this->hwnd, SW_MAXIMIZE);
}
void GTKWindowWin32::Show() {
	ShowWindow(this->hwnd, SW_SHOW);
}
void GTKWindowWin32::Hide() {
	ShowWindow(this->hwnd, SW_HIDE);
}

int GTKWindowWin32::GetState(void) const {
	return this->state;
}

void GTKWindowWin32::GetPos(int *x, int *y) const {
	RECT rect;
	if (::GetWindowRect(this->hwnd, &rect)) {
		if (x) {
			*x = rect.left;
		}
		if (y) {
			*y = rect.top;
		}
	}
}

void GTKWindowWin32::GetSize(int *w, int *h) const {
	RECT rect;
	if (::GetClientRect(this->hwnd, &rect)) {
		if (w) {
			*w = rect.right - rect.left;
		}
		if (h) {
			*h = rect.bottom - rect.top;
		}
	}
}

HWND GTKWindowWin32::GetHandle(void) {
	return this->hwnd;
}

HDC GTKWindowWin32::GetDeviceHandle() {
	return this->device;
}

void GTKWindowWin32::ThemeRefresh() {
	DWORD lightMode;
	DWORD pcbData = sizeof(lightMode);
	if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize\\", L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &lightMode, &pcbData) == ERROR_SUCCESS) {
		BOOL DarkMode = !lightMode;
		/* `20 == DWMWA_USE_IMMERSIVE_DARK_MODE` in Windows 11 SDK.
		 * This value was undocumented for Windows 10 versions 2004 and later,
		 * supported for Windows 11 Build 22000 and later. */
		DwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &DarkMode, sizeof(DarkMode));
	}
}

GTKRenderInterface *GTKWindowWin32::AllocateRender(int backend) {
	switch (backend) {
		case GTK_WINDOW_RENDER_OPENGL: {
			return static_cast<GTKRenderInterface *>(MEM_new<GTKRenderWGL>("GTKRenderWGL", this));
		} break;
	}
	return NULL;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam) {
	GTKManagerWin32 *manager = (GTKManagerWin32 *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (!manager) {
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	GTKWindowWin32 *window = manager->GetWindowByHandle(hwnd);

	if (!window) {
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	float time = static_cast<float>(GetMessageTime()) / 1000.0;

	switch (message) {
		case WM_DESTROY: {
			if (manager->DispatchDestroy) {
				manager->DispatchDestroy(window);
			}
		} break;
		case WM_SETTINGCHANGE:
		case WM_SYSCOLORCHANGE: {
			window->ThemeRefresh();
		} break;
		case WM_SIZE: {
			int w = LOWORD(lparam);
			int h = HIWORD(lparam);

			switch (wparam) {
				case SIZE_MAXIMIZED: {
					window->state = GTK_STATE_MAXIMIZED;
				} break;
				case SIZE_MINIMIZED: {
					window->state = GTK_STATE_MINIMIZED;
				} break;
				case SIZE_RESTORED: {
					window->state = GTK_STATE_RESTORED;
				} break;
			}

			if (manager->DispatchResize) {
				manager->DispatchResize(window, w, h);
			}
		} break;
		case WM_MOVE: {
			int x = LOWORD(lparam);
			int y = HIWORD(lparam);

			if (manager->DispatchMove) {
				manager->DispatchMove(window, x, y);
			}
		} break;
		case WM_ACTIVATE: {
			if (wparam != WA_INACTIVE) {
				if (manager->DispatchFocus) {
					manager->DispatchFocus(window, false);
				}
			} else {
				if (manager->DispatchFocus) {
					manager->DispatchFocus(window, true);
				}
			}
		} break;
		case WM_INPUTLANGCHANGE: {
			manager->keyboard_layout = (HKL)lparam;
		} break;
		case WM_INPUT: {
			if (GetActiveWindow() != hwnd) {
				break;
			}

			RAWINPUT raw;
			UINT raw_size = sizeof(RAWINPUT);

			::GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &raw, &raw_size, sizeof(RAWINPUTHEADER));

			switch (raw.header.dwType) {
				case RIM_TYPEKEYBOARD: {
					const USHORT virtual_key = raw.data.keyboard.VKey;
					const USHORT make_code = raw.data.keyboard.MakeCode;
					const USHORT flags = raw.data.keyboard.Flags;
					const UINT msg = raw.data.keyboard.Message;

					bool keydown = !(raw.data.keyboard.Flags & RI_KEY_BREAK) && msg != WM_KEYUP && msg != WM_SYSKEYUP;
					bool repeat = false;

					int tiny_window_key = ConvertInputKey(virtual_key, make_code, flags & (RI_KEY_E1 | RI_KEY_E0));

					if (keydown) {
						if (HIBYTE(GetKeyState(virtual_key)) != 0) {
							repeat = true;
						}
					}

					wchar_t utf16[2] = {0};
					char utf8[4] = {0};
					BYTE state[256];

					const BOOL has_state = ::GetKeyboardState((PBYTE)state);
					const BOOL has_ctrl = has_state && state[VK_CONTROL] & 0x80;
					const BOOL has_alt = has_state && state[VK_MENU] & 0x80;

					if (has_ctrl && !has_alt) {
						/** No text with control key pressed (Alt can be used to insert special characters though!). */
					}
					else if (::MapVirtualKeyW(virtual_key, MAPVK_VK_TO_CHAR) != 0) {
						INT ret;
						if ((ret = ::ToUnicodeEx(virtual_key, raw.data.keyboard.MakeCode, state, utf16, 2, 0, manager->keyboard_layout)) > 0) {
							ret = ::WideCharToMultiByte(CP_UTF8, 0, utf16, ret, utf8, sizeof(utf8), NULL, NULL);
							if (0 <= ret && ret < sizeof(utf8) / sizeof(utf8[0])) {
								utf8[ret] = '\0';
							}
						}
					}

					if (keydown && manager->DispatchKeyDown) {
						manager->DispatchKeyDown(window, tiny_window_key, repeat, utf8, time);
					}
					if (!keydown && manager->DispatchKeyUp) {
						manager->DispatchKeyUp(window, tiny_window_key, time);
					}
				} break;
			}
		} break;
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN: {
			if (GetActiveWindow() != hwnd) {
				break;
			}

			SetCapture(hwnd);

			int x = static_cast<int>(GET_X_LPARAM(lparam));
			int y = static_cast<int>(GET_Y_LPARAM(lparam));

			switch (message) {
				case WM_LBUTTONDOWN: {
					if (manager->DispatchButtonDown) {
						manager->DispatchButtonDown(window, GTK_BTN_LEFT, x, y, time);
					}
				} break;
				case WM_MBUTTONDOWN: {
					if (manager->DispatchButtonDown) {
						manager->DispatchButtonDown(window, GTK_BTN_MIDDLE, x, y, time);
					}
				} break;
				case WM_RBUTTONDOWN: {
					if (manager->DispatchButtonDown) {
						manager->DispatchButtonDown(window, GTK_BTN_RIGHT, x, y, time);
					}
				} break;
			}
		} break;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP: {
			if (GetActiveWindow() != hwnd) {
				break;
			}

			ReleaseCapture();

			int x = static_cast<int>(GET_X_LPARAM(lparam));
			int y = static_cast<int>(GET_Y_LPARAM(lparam));

			switch (message) {
				case WM_LBUTTONUP: {
					if (manager->DispatchButtonUp) {
						manager->DispatchButtonUp(window, GTK_BTN_LEFT, x, y, time);
					}
				} break;
				case WM_MBUTTONUP: {
					if (manager->DispatchButtonUp) {
						manager->DispatchButtonUp(window, GTK_BTN_MIDDLE, x, y, time);
					}
				} break;
				case WM_RBUTTONUP: {
					if (manager->DispatchButtonUp) {
						manager->DispatchButtonUp(window, GTK_BTN_RIGHT, x, y, time);
					}
				} break;
			}
		} break;
		case WM_MOUSELEAVE: {
			if (GetActiveWindow() != hwnd) {
				break;
			}

			POINT xy;
			::GetCursorPos(&xy);
			::ScreenToClient(hwnd, &xy);

			if (manager->DispatchMouse) {
				manager->DispatchMouse(window, xy.x, xy.y, time);
			}
		} break;
		case WM_MOUSEMOVE: {
			if (GetActiveWindow() != hwnd) {
				break;
			}

			TRACKMOUSEEVENT track;
			track.cbSize = sizeof(TRACKMOUSEEVENT);
			track.dwFlags = TME_LEAVE;
			track.hwndTrack = hwnd;
			track.dwHoverTime = HOVER_DEFAULT;
			::TrackMouseEvent(&track);

			int x = static_cast<int>(GET_X_LPARAM(lparam));
			int y = static_cast<int>(GET_Y_LPARAM(lparam));

			if (manager->DispatchMouse) {
				manager->DispatchMouse(window, x, y, time);
			}
		} break;
		case WM_MOUSEWHEEL: {
			if (GetActiveWindow() != hwnd) {
				break;
			}

			int delta = GET_WHEEL_DELTA_WPARAM(wparam);

			if (manager->DispatchWheel) {
				manager->DispatchWheel(window, 0, delta / WHEEL_DELTA, time);
			}
		} break;
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}
