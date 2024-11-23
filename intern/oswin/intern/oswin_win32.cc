#include "oswin.h"
#include "oswin.hh"

#include <assert.h>
#include <windowsx.h>

#define SET_FLAG_FROM_TEST(value, test, flag) \
	{                                         \
		if (test) {                           \
			(value) |= (flag);                \
		}                                     \
		else {                                \
			(value) &= ~(flag);               \
		}                                     \
	}                                         \
	((void)0)

namespace rose::tiny_window {

static int convert_key(short win32_key, short scan_code, short extend);

tWindowManager::tWindowManager() {
	this->start_ = std::chrono::high_resolution_clock::now();
}

tWindowManager::~tWindowManager() {
	ClearGamepad();
	ClearExtensions();
	ClearDummy();
	ClearKeyboardLayout();
	ClearScreenInfo();

	for (FormatSetting *format : formats_) {
		MEM_delete<FormatSetting>(format);
	}
	formats_.clear();
}

/* -------------------------------------------------------------------- */
/** \name [WindowManager] Win32 Public Methods
 * \{ */

tWindowManager *tWindowManager::Init(const char *application) {
	tWindowManager *manager = new tWindowManager;

	if (!manager->GetScreenInfo()) {
		delete manager;
		return nullptr;
	}
	if (!manager->InitKeyboardLayout()) {
		delete manager;
		return nullptr;
	}
	if (!manager->InitDummy()) {
		delete manager;
		return nullptr;
	}
	if (!manager->InitExtensions()) {
		delete manager;
		return nullptr;
	}
	if (!manager->InitGamepad()) {
		delete manager;
		return nullptr;
	}
	if (!::SetConsoleOutputCP(CP_UTF8)) {
		// delete manager;
		// return nullptr;
	}

	return manager;
}

bool tWindowManager::HasEventsWaiting() {
	MSG msg;
	if (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
		return true;
	}
	return false;
}
void tWindowManager::Poll() {
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	PollGamepads();
}

bool tWindowManager::SetClipboard(const char *buffer, unsigned int length, bool selection) const {
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

bool tWindowManager::GetClipboard(char **r_buffer, unsigned int *r_length, bool selection) const {
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

		*r_buffer = static_cast<char *>(MEM_mallocN(ret + 1, "win32::clipboard"));
		*r_length = static_cast<unsigned int>(ret);

		if (::WideCharToMultiByte(CP_UTF8, 0, buffer, ret, *r_buffer, *r_length, NULL, NULL) < 0) {
			r_buffer[0] = '\0';
		}

		*r_length = strlen(*r_buffer);

		GlobalUnlock(handle);
		CloseClipboard();

		return true;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name [WindowManager] Win32 Private Methods
 * \{ */

static int GetNumberOfGamepads(void) {
	int number = 0;
	for (int iter = 0; iter < XUSER_MAX_COUNT; iter++) {
		XINPUT_STATE state;
		ZeroMemory(&state, sizeof(XINPUT_STATE));

		if (XInputGetState(iter, &state) == ERROR_SUCCESS) {
			number = std::max(number, iter);
		}
	}
	return number;
}

bool tWindowManager::GetScreenInfo() {
	DISPLAY_DEVICE graphicsDevice;
	graphicsDevice.cb = sizeof(DISPLAY_DEVICE);
	graphicsDevice.StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
	DWORD nDevice = 0;
	DWORD nMonitor = 0;
	while (EnumDisplayDevices(nullptr, nDevice, &graphicsDevice, EDD_GET_DEVICE_INTERFACE_NAME)) {
		// get monitor info for the current display device
		DISPLAY_DEVICE monitorDevice = {0};
		monitorDevice.cb = sizeof(DISPLAY_DEVICE);
		monitorDevice.StateFlags = DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
		tMonitor *monitor = nullptr;

		// if it has children add them to the list, else, ignore them since those are only POTENTIAL monitors/devices
		nMonitor = 0;
		while (EnumDisplayDevices(graphicsDevice.DeviceName, nMonitor, &monitorDevice, EDD_GET_DEVICE_INTERFACE_NAME)) {
			monitor = MEM_new<tMonitor>("tMonitor");
			monitor->monitor = graphicsDevice.DeviceString;
			monitor->device = graphicsDevice.DeviceString;
			monitor->name = graphicsDevice.DeviceName;
			// get current display mode
			DEVMODE devmode = {0};
			// get all display modes
			unsigned int modeIndex = UINT_MAX;
			while (EnumDisplaySettings(graphicsDevice.DeviceName, modeIndex, &devmode)) {
				MonitorSetting *setting = MEM_new<MonitorSetting>("MonitorSetting");
				setting->sizex = devmode.dmPelsWidth;
				setting->sizey = devmode.dmPelsHeight;
				setting->bpp = devmode.dmBitsPerPel;
				setting->frequency = devmode.dmDisplayFrequency;

				// get the current settings of the display
				if (modeIndex == ENUM_CURRENT_SETTINGS) {
					monitor->current = setting;
				}
				// get the settings that are stored in the registry
				else {
					monitor->settings.push_back(setting);
				}
				modeIndex++;
			}
			monitors_.push_back(monitor);
			nMonitor++;
		}
		nDevice++;
	}
	return true;
}
void tWindowManager::ClearScreenInfo() {
	for (auto &monitor : monitors_) {
		for (auto &setting : monitor->settings) {
			MEM_delete<MonitorSetting>(setting);
		}
		MEM_delete<MonitorSetting>(monitor->current);
		MEM_delete<tMonitor>(monitor);
	}
	monitors_.clear();
}

int tWindowManager::GetLegacyPFD(const FormatSetting *desired, HDC hdc) {
	int nNative = 0;
	int nCompatible = 0;

	nNative = DescribePixelFormat(hdc, 1, sizeof(PIXELFORMATDESCRIPTOR), NULL);

	for (FormatSetting *format : formats_) {
		MEM_delete<FormatSetting>(format);
	}
	formats_.clear();

	for (int iNative = 1; iNative <= nNative; iNative++) {
		PIXELFORMATDESCRIPTOR pfd;
		if (!DescribePixelFormat(hdc, iNative, sizeof(PIXELFORMATDESCRIPTOR), &pfd)) {
			continue;
		}

		// skip if the PFD does not have PFD_DRAW_TO_WINDOW and PFD_SUPPORT_OPENGL
		if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW) || !(pfd.dwFlags & PFD_SUPPORT_OPENGL)) {
			continue;
		}
		// skip if the PFD does not have PFD_GENERIC_ACCELERATION and PFD_GENERIC FORMAT
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

		formats_.push_back(format);
		nCompatible++;
	}

	FormatSetting *format = GetClosestFormat(desired);

	if (format) {
		return format->handle;
	}
	return 0;
}
int tWindowManager::FillGamepad(XINPUT_STATE state, int index) {
	if (index >= gamepads_.size()) {
		return -1;
	}

	gamepads_[index].buttons[Gamepad::dpad_top] = state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
	gamepads_[index].buttons[Gamepad::dpad_bottom] = state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
	gamepads_[index].buttons[Gamepad::dpad_left] = state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
	gamepads_[index].buttons[Gamepad::dpad_right] = state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

	gamepads_[index].buttons[Gamepad::start] = state.Gamepad.wButtons & XINPUT_GAMEPAD_START;
	gamepads_[index].buttons[Gamepad::select] = state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;

	gamepads_[index].buttons[Gamepad::left_stick] = state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
	gamepads_[index].buttons[Gamepad::right_stick] = state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;

	gamepads_[index].buttons[Gamepad::left_shoulder] = state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
	gamepads_[index].buttons[Gamepad::right_shoulder] = state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

	gamepads_[index].buttons[Gamepad::face_bottom] = state.Gamepad.wButtons & XINPUT_GAMEPAD_A;
	gamepads_[index].buttons[Gamepad::face_right] = state.Gamepad.wButtons & XINPUT_GAMEPAD_B;
	gamepads_[index].buttons[Gamepad::face_left] = state.Gamepad.wButtons & XINPUT_GAMEPAD_X;
	gamepads_[index].buttons[Gamepad::face_top] = state.Gamepad.wButtons & XINPUT_GAMEPAD_Y;

	gamepads_[index].triggers.left = state.Gamepad.bLeftTrigger / std::numeric_limits<unsigned char>::max();
	gamepads_[index].triggers.right = state.Gamepad.bRightTrigger / std::numeric_limits<unsigned char>::max();

	gamepads_[index].lstick[0] = state.Gamepad.sThumbLX / std::numeric_limits<short>::max();
	gamepads_[index].lstick[1] = state.Gamepad.sThumbLY / std::numeric_limits<short>::max();
	gamepads_[index].rstick[0] = state.Gamepad.sThumbRX / std::numeric_limits<short>::max();
	gamepads_[index].rstick[1] = state.Gamepad.sThumbRY / std::numeric_limits<short>::max();
	return index;
}
int tWindowManager::PollGamepads() {
	this->gamepads_.resize(GetNumberOfGamepads());
	for (int iter = 0; iter < XUSER_MAX_COUNT; iter++) {
		XINPUT_STATE state;
		ZeroMemory(&state, sizeof(XINPUT_STATE));

		if (XInputGetState((DWORD)iter, &state) != ERROR_SUCCESS) {
			return iter;
		}

		FillGamepad(state, iter);
	}
	return XUSER_MAX_COUNT;
}

bool tWindowManager::InitKeyboardLayout() {
	RAWINPUTDEVICE device;
	memset(&device, 0, sizeof(RAWINPUTDEVICE));

	/* http://msdn.microsoft.com/en-us/windows/hardware/gg487473.aspx */
	device.usUsagePage = 0x01;
	device.usUsage = 0x06;

	if (RegisterRawInputDevices(&device, 1, sizeof(RAWINPUTDEVICE))) {
		this->keyboard_layout_ = ::GetKeyboardLayout(0);
		return true;
	}
	return false;
}
void tWindowManager::ClearKeyboardLayout() {
}

bool tWindowManager::InitDummy() {
	dummy.instance_handle_ = GetModuleHandle(NULL);

	dummy.window_class_.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	dummy.window_class_.lpfnWndProc = tWindowManager::WindowProcedure;
	dummy.window_class_.cbClsExtra = 0;
	dummy.window_class_.cbWndExtra = 0;
	dummy.window_class_.hInstance = dummy.instance_handle_;
	dummy.window_class_.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	dummy.window_class_.hCursor = LoadCursor(NULL, IDC_ARROW);
	dummy.window_class_.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	dummy.window_class_.lpszMenuName = NULL;
	dummy.window_class_.lpszClassName = "dummy";
	RegisterClass(&dummy.window_class_);

	dummy.window_handle_ = CreateWindowEx(WS_EX_APPWINDOW, "dummy", "dummy", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1, 1, HWND_DESKTOP, NULL, dummy.instance_handle_, NULL);

	if (!dummy.window_handle_) {
		return false;
	}

	ShowWindow(dummy.window_handle_, SW_HIDE);

	dummy.device_handle_ = GetDC(dummy.window_handle_);
	if (!dummy.device_handle_) {
		return false;
	}

	FormatSetting format = {};
	int handle = GetLegacyPFD(&format, dummy.device_handle_);

	PIXELFORMATDESCRIPTOR pfd;
	if (!DescribePixelFormat(dummy.device_handle_, handle, sizeof(PIXELFORMATDESCRIPTOR), &pfd)) {
		return false;
	}
	if (!SetPixelFormat(dummy.device_handle_, handle, &pfd)) {
		return false;
	}

	dummy.rendering_handle_ = wglCreateContext(dummy.device_handle_);
	if (!dummy.rendering_handle_) {
		return false;
	}
	if (!wglMakeCurrent(dummy.device_handle_, dummy.rendering_handle_)) {
		return false;
	}

	return true;
}
void tWindowManager::ClearDummy() {
	if (dummy.rendering_handle_) {
		wglDeleteContext(dummy.rendering_handle_);
	}
	if (dummy.device_handle_) {
		ReleaseDC(dummy.window_handle_, dummy.device_handle_);
	}
	if (dummy.window_handle_) {
		DestroyWindow(dummy.window_handle_);
	}
	UnregisterClass("NotAnotherDummy", dummy.instance_handle_);
}

bool tWindowManager::ExtensionSupported(const char *extensionName) {
	const char *wglExtensions;

	if (wglGetExtensionsStringARB != nullptr) {
		wglExtensions = wglGetExtensionsStringARB(this->dummy.device_handle_);
		if (wglExtensions != nullptr) {
			if (std::strstr(wglExtensions, extensionName) != nullptr) {
				return true;
			}
		}
	}

	if (wglGetExtensionsStringEXT != nullptr) {
		wglExtensions = wglGetExtensionsStringEXT();
		if (wglExtensions != nullptr) {
			if (std::strstr(wglExtensions, extensionName) != nullptr) {
				return true;
			}
		}
	}
	return false;
}

bool tWindowManager::InitExtensions() {
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

	this->has_swap_interval_control_ = ExtensionSupported("WGL_EXT_swap_control");
	this->has_srgb_framebuffer_support_ = ExtensionSupported("WGL_ARB_framebuffer_sRGB");

	wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribfvARB");
	wglGetPixelFormatAttribfvEXT = (PFNWGLGETPIXELFORMATATTRIBFVEXTPROC)wglGetProcAddress("wglGetPixelFormatAttribfvEXT");
	wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)wglGetProcAddress("wglGetPixelFormatAttribivARB");
	wglGetPixelFormatAttribivEXT = (PFNWGLGETPIXELFORMATATTRIBIVEXTPROC)wglGetProcAddress("wglGetPixelFormatAttribivEXT");
	return true;
}
void tWindowManager::ClearExtensions() {
}

bool tWindowManager::InitGamepad() {
	this->gamepads_.resize(GetNumberOfGamepads());
	for (int iter = 0; iter < XUSER_MAX_COUNT; iter++) {
		XINPUT_STATE state;
		ZeroMemory(&state, sizeof(XINPUT_STATE));

		if (XInputGetState((DWORD)iter, &state) != ERROR_SUCCESS) {
			continue;
		}

		FillGamepad(state, iter);
	}
	return true;
}
void tWindowManager::ClearGamepad() {
}

tWindow *tWindowManager::GetWindowByHandle(HWND handle) const {
	for (auto itr = this->windows_.begin(); itr != this->windows_.end(); itr++) {
		if ((*itr)->window_handle_ == handle) {
			return *itr;
		}
	}
	return nullptr;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name [Window] Win32 Public Methods
 * \{ */

void tWindow::SetWindowStyle(Style style) {
	switch (style) {
		case Style::normal: {
			SetWindowDecoration(Decorator::titlebar | Decorator::border | Decorator::destroy_btn | Decorator::minimize_btn | Decorator::maximize_btn | Decorator::sizeable, true);
			break;
		}

		case Style::popup: {
			SetWindowDecoration(Decorator::none, true);
			break;
		}

		case Style::bare: {
			SetWindowDecoration(Decorator::titlebar | Decorator::border, true);
			break;
		}
	}
}
void tWindow::SetWindowDecoration(Decorator decoration, bool add) {
	ULONG style = WS_CLIPCHILDREN | WS_VISIBLE;

	if (!add) {
		style = GetWindowLong(this->window_handle_, GWL_STYLE);
	}

	if ((decoration & Decorator::border) != Decorator::none) {
		SET_FLAG_FROM_TEST(style, add, WS_BORDER);
	}
	if ((decoration & Decorator::titlebar) != Decorator::none) {
		SET_FLAG_FROM_TEST(style, add, WS_CAPTION);
	}
	if ((decoration & Decorator::icon) != Decorator::none) {
		SET_FLAG_FROM_TEST(style, add, WS_ICONIC);
	}
	if ((decoration & Decorator::destroy_btn) != Decorator::none) {
		SET_FLAG_FROM_TEST(style, add, WS_SYSMENU);
	}
	if ((decoration & Decorator::minimize_btn) != Decorator::none) {
		SET_FLAG_FROM_TEST(style, add, WS_MINIMIZEBOX | WS_SYSMENU);
	}
	if ((decoration & Decorator::maximize_btn) != Decorator::none) {
		SET_FLAG_FROM_TEST(style, add, WS_MAXIMIZEBOX | WS_SYSMENU);
	}
	if ((decoration & Decorator::sizeable) != Decorator::none) {
		SET_FLAG_FROM_TEST(style, add, WS_SIZEBOX);
	}

	SetWindowLong(this->window_handle_, GWL_STYLE, style);
	SetWindowPos(this->window_handle_, HWND_TOP, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
}
void tWindow::SetTitleBar(const char *title) {
}

State tWindow::GetState() const {
	return this->state_;
}

void tWindow::Show() {
	ShowWindow(this->window_handle_, SW_SHOW);
}
void tWindow::Hide() {
	ShowWindow(this->window_handle_, SW_HIDE);
}

bool tWindow::MakeContextCurrent() {
	return wglMakeCurrent(this->device_handle_, this->rendering_handle_);
}
bool tWindow::SetSwapInterval(int interval) {
	tWindowManager *manager = (tWindowManager *)GetWindowLongPtr(this->window_handle_, GWLP_USERDATA);
	if (!manager) {
		return false;
	}

	if (manager->has_swap_interval_control_ && manager->wglSwapIntervalEXT != nullptr) {
		HGLRC prev_rendering_handle = wglGetCurrentContext();
		HDC prev_device_handle = wglGetCurrentDC();
		wglMakeCurrent(this->device_handle_, this->rendering_handle_);
		manager->wglSwapIntervalEXT(interval);
		wglMakeCurrent(prev_device_handle, prev_rendering_handle);
		return true;
	}
	return false;
}
bool tWindow::GetSwapInterval(int *interval) {
	tWindowManager *manager = (tWindowManager *)GetWindowLongPtr(this->window_handle_, GWLP_USERDATA);
	if (!manager) {
		return false;
	}

	if (manager->has_swap_interval_control_ && manager->wglGetSwapIntervalEXT != nullptr) {
		HGLRC prev_rendering_handle = wglGetCurrentContext();
		HDC prev_device_handle = wglGetCurrentDC();
		wglMakeCurrent(this->device_handle_, this->rendering_handle_);
		*interval = manager->wglGetSwapIntervalEXT();
		wglMakeCurrent(prev_device_handle, prev_rendering_handle);
		return true;
	}
	return false;
}
bool tWindow::SwapBuffers() {
	return ::SwapBuffers(this->device_handle_);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name [Window] Win32 Private Methods
 * \{ */

tWindow::tWindow(WindowSetting settings) : settings(settings) {
}

tWindow::~tWindow() {
	SetWindowLongPtr(this->window_handle_, GWLP_USERDATA, (LONG_PTR)NULL);

	if (this->rendering_handle_) {
		wglDeleteContext(this->rendering_handle_);
	}
	if (this->device_handle_) {
		ReleaseDC(this->window_handle_, this->device_handle_);
	}
	if (this->window_handle_) {
		/**
		 * Calling #DestroyWindow would send an message to WindowProcedure,
		 * that message would be sent to the user, if the user called this
		 * then that message should not be sent back to the user again!
		 */
		DefWindowProc(this->window_handle_, WM_DESTROY, NULL, NULL);
	}
	UnregisterClass(this->window_class_.lpszClassName, this->window_class_.hInstance);
}

bool tWindow::Init(tWindowManager *manager) {
	this->instance_handle_ = GetModuleHandleA(NULL);

	this->window_class_.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	this->window_class_.lpfnWndProc = tWindowManager::WindowProcedure;
	this->window_class_.cbClsExtra = 0;
	this->window_class_.cbWndExtra = 0;
	this->window_class_.hInstance = this->instance_handle_;
	this->window_class_.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	this->window_class_.hCursor = LoadCursor(nullptr, IDC_ARROW);
	this->window_class_.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	this->window_class_.lpszMenuName = NULL;
	this->window_class_.lpszClassName = settings.name;
	RegisterClass(&this->window_class_);

	DWORD style = WS_OVERLAPPEDWINDOW & ~WS_VISIBLE;

	this->window_handle_ = CreateWindowEx(WS_EX_APPWINDOW, settings.name, settings.name, style, CW_USEDEFAULT, CW_USEDEFAULT, settings.width, settings.height, HWND_DESKTOP, NULL, this->instance_handle_, NULL);

	if (!this->window_handle_) {
		return false;
	}

	SetWindowLongPtr(this->window_handle_, GWLP_USERDATA, (LONG_PTR)manager);

	if (!(this->context_created_ = InitContext(manager))) {
		return false;
	}

	RECT tempRect;
	::GetWindowRect(this->window_handle_, &tempRect);
	this->sizex = tempRect.right - tempRect.left;
	this->sizey = tempRect.bottom - tempRect.top;

	::GetClientRect(this->window_handle_, &tempRect);
	this->clientx = tempRect.right - tempRect.left;
	this->clienty = tempRect.bottom - tempRect.top;
	return true;
}
bool tWindow::InitContext(tWindowManager *manager) {
	if (!manager) {
		return false;
	}

	this->device_handle_ = GetDC(this->window_handle_);

	if (!InitPixelFormat()) {
		return false;
	}

	if (manager->wglCreateContextAttribsARB) {
		int attribs[]{WGL_CONTEXT_MAJOR_VERSION_ARB, settings.major, WGL_CONTEXT_MINOR_VERSION_ARB, settings.minor, 0};

		this->rendering_handle_ = wglCreateContext(this->device_handle_);

		if (!this->rendering_handle_) {
			return false;
		}
	}
	else {
		this->rendering_handle_ = wglCreateContext(this->device_handle_);
	}

	wglMakeCurrent(this->device_handle_, this->rendering_handle_);

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return false;
	}

	return this->context_created_ = true;
}
bool tWindow::InitPixelFormat() {
	tWindowManager *manager = (tWindowManager *)GetWindowLongPtr(this->window_handle_, GWLP_USERDATA);
	if (!manager) {
		return false;
	}

	unsigned int count = WGL_NUMBER_PIXEL_FORMATS_ARB;
	int index = 0;
	int attribs[] = {WGL_SUPPORT_OPENGL_ARB, 1, WGL_DRAW_TO_WINDOW_ARB, 1, WGL_DOUBLE_BUFFER_ARB, 1, WGL_RED_BITS_ARB, settings.color_bits, WGL_GREEN_BITS_ARB, settings.color_bits, WGL_BLUE_BITS_ARB, settings.color_bits, WGL_ALPHA_BITS_ARB, settings.color_bits, WGL_DEPTH_BITS_ARB, settings.depth_bits, WGL_STENCIL_BITS_ARB, settings.stencil_bits, WGL_ACCUM_RED_BITS_ARB, settings.accum_bits, WGL_ACCUM_GREEN_BITS_ARB, settings.accum_bits, WGL_ACCUM_BLUE_BITS_ARB, settings.accum_bits, WGL_ACCUM_ALPHA_BITS_ARB, settings.accum_bits, WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB, WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB};

	std::vector<int> attribList;
	attribList.assign(attribs, attribs + std::size(attribs));

	PIXELFORMATDESCRIPTOR pfd = {};
	if (wglChoosePixelFormatARB != nullptr) {
		if (settings.srgb) {
			attribList.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB);
		}

		attribList.push_back(0);  // needs a 0 to notify as the end of the list.
		wglChoosePixelFormatARB(this->device_handle_, &attribList[0], nullptr, 1, &index, &count);
		if (DescribePixelFormat(this->device_handle_, index, sizeof(pfd), &pfd)) {
			SetPixelFormat(this->device_handle_, index, &pfd);
			return true;
		}
	}
	if (wglChoosePixelFormatEXT != nullptr) {
		if (settings.srgb) {
			attribList.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT);
		}

		attribList.push_back(0);
		wglChoosePixelFormatEXT(this->device_handle_, &attribList[0], nullptr, 1, &index, &count);
		if (DescribePixelFormat(this->device_handle_, index, sizeof(pfd), &pfd)) {
			SetPixelFormat(this->device_handle_, index, &pfd);
			return true;
		}
	}
	FormatSetting format = {};
	index = manager->GetLegacyPFD(&format, this->device_handle_);
	if (!DescribePixelFormat(this->device_handle_, index, sizeof(pfd), &pfd)) {
		return false;
	}
	SetPixelFormat(this->device_handle_, index, &pfd);
	return true;
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

static int convert_key(short win32_key, short scan_code, short extend) {
	int key = WTK_KEY_UNKOWN;

	if ((win32_key >= '0') && (win32_key <= '9')) {
		key = (win32_key - '0' + WTK_KEY_0);
	}
	else if ((win32_key >= 'A') && (win32_key <= 'Z')) {
		key = (win32_key - 'A' + WTK_KEY_A);
	}
	else if ((win32_key >= VK_F1) && (win32_key <= VK_F24)) {
		key = (win32_key - VK_F1 + WTK_KEY_F1);
	}
	else {
		switch (win32_key) {
			case VK_RETURN: {
				key = (extend) ? WTK_KEY_NUMPAD_ENTER : WTK_KEY_ENTER;
			} break;
			case VK_SPACE: {
				key = WTK_KEY_SPACE;
			} break;
			case VK_BACK: {
				key = WTK_KEY_BACKSPACE;
			} break;
			case VK_TAB: {
				key = WTK_KEY_TAB;
			} break;
			case VK_ESCAPE: {
				key = WTK_KEY_ESC;
			} break;
			case VK_INSERT:
			case VK_NUMPAD0: {
				key = (extend) ? WTK_KEY_INSERT : WTK_KEY_NUMPAD_0;
			} break;
			case VK_END:
			case VK_NUMPAD1: {
				key = (extend) ? WTK_KEY_END : WTK_KEY_NUMPAD_1;
			} break;
			case VK_DOWN:
			case VK_NUMPAD2: {
				key = (extend) ? WTK_KEY_DOWN : WTK_KEY_NUMPAD_2;
			} break;
			case VK_NEXT:
			case VK_NUMPAD3: {
				key = (extend) ? WTK_KEY_PAGEDOWN : WTK_KEY_NUMPAD_3;
			} break;
			case VK_LEFT:
			case VK_NUMPAD4: {
				key = (extend) ? WTK_KEY_LEFT : WTK_KEY_NUMPAD_4;
			} break;
			case VK_CLEAR:
			case VK_NUMPAD5: {
				key = (extend) ? WTK_KEY_UNKOWN : WTK_KEY_NUMPAD_5;
			} break;
			case VK_RIGHT:
			case VK_NUMPAD6: {
				key = (extend) ? WTK_KEY_RIGHT : WTK_KEY_NUMPAD_6;
			} break;
			case VK_HOME:
			case VK_NUMPAD7: {
				key = (extend) ? WTK_KEY_HOME : WTK_KEY_NUMPAD_7;
			} break;
			case VK_UP:
			case VK_NUMPAD8: {
				key = (extend) ? WTK_KEY_UP : WTK_KEY_NUMPAD_8;
			} break;
			case VK_PRIOR:
			case VK_NUMPAD9: {
				key = (extend) ? WTK_KEY_PAGEUP : WTK_KEY_NUMPAD_9;
			} break;
			case VK_DECIMAL:
			case VK_DELETE: {
				key = (extend) ? WTK_KEY_DELETE : WTK_KEY_NUMPAD_PERIOD;
			} break;

			case VK_SNAPSHOT: {
				key = WTK_KEY_PRTSCN;
			} break;
			case VK_PAUSE: {
				key = WTK_KEY_PAUSE;
			} break;
			case VK_MULTIPLY: {
				key = WTK_KEY_NUMPAD_ASTERISK;
			} break;
			case VK_SUBTRACT: {
				key = WTK_KEY_NUMPAD_MINUS;
			} break;
			case VK_DIVIDE: {
				key = WTK_KEY_NUMPAD_SLASH;
			} break;
			case VK_ADD: {
				key = WTK_KEY_NUMPAD_PLUS;
			} break;

			case VK_SEMICOLON: {
				key = WTK_KEY_SEMICOLON;
			} break;
			case VK_EQUALS: {
				key = WTK_KEY_EQUAL;
			} break;
			case VK_COMMA: {
				key = WTK_KEY_COMMA;
			} break;
			case VK_MINUS: {
				key = WTK_KEY_MINUS;
			} break;
			case VK_PERIOD: {
				key = WTK_KEY_PERIOD;
			} break;
			case VK_SLASH: {
				key = WTK_KEY_SLASH;
			} break;
			case VK_BACK_QUOTE: {
				key = WTK_KEY_ACCENTGRAVE;
			} break;
			case VK_OPEN_BRACKET: {
				key = WTK_KEY_LEFT_BRACKET;
			} break;
			case VK_BACK_SLASH: {
				key = WTK_KEY_BACKSLASH;
			} break;
			case VK_CLOSE_BRACKET: {
				key = WTK_KEY_RIGHT_BRACKET;
			} break;
			case VK_GR_LESS: {
				key = WTK_KEY_GRLESS;
			} break;

			case VK_SHIFT: {
				if (scan_code == 0x36) {
					key = WTK_KEY_RIGHT_SHIFT;
				}
				else if (scan_code == 0x2a) {
					key = WTK_KEY_LEFT_SHIFT;
				}
				else {
					key = WTK_KEY_UNKOWN;
				}
			} break;
			case VK_CONTROL: {
				key = (extend) ? WTK_KEY_RIGHT_CTRL : WTK_KEY_LEFT_CTRL;
			} break;
			case VK_MENU: {
				key = (extend) ? WTK_KEY_RIGHT_ALT : WTK_KEY_LEFT_ALT;
			} break;
			case VK_LWIN: {
				key = WTK_KEY_LEFT_OS;
			} break;
			case VK_RWIN: {
				key = WTK_KEY_RIGHT_OS;
			} break;
			case VK_APPS: {
				key = WTK_KEY_APP;
			} break;
			case VK_NUMLOCK: {
				key = WTK_KEY_NUMLOCK;
			} break;
			case VK_SCROLL: {
				key = WTK_KEY_SCRLOCK;
			} break;
			case VK_CAPITAL: {
				key = WTK_KEY_CAPSLOCK;
			} break;
			case VK_MEDIA_PLAY_PAUSE: {
				key = WTK_KEY_MEDIA_PLAY;
			} break;
			case VK_MEDIA_STOP: {
				key = WTK_KEY_MEDIA_STOP;
			} break;
			case VK_MEDIA_PREV_TRACK: {
				key = WTK_KEY_MEDIA_FIRST;
			} break;
			case VK_MEDIA_NEXT_TRACK: {
				key = WTK_KEY_MEDIA_LAST;
			} break;
		}
	}

	return key;
}

/** \} */

}  // namespace rose::tiny_window

LRESULT CALLBACK rose::tiny_window::tWindowManager::WindowProcedure(HWND hwnd, unsigned int message, WPARAM wparam, LPARAM lparam) {
	rose::tiny_window::tWindowManager *manager = (rose::tiny_window::tWindowManager *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	rose::tiny_window::tWindow *window = (manager) ? manager->GetWindowByHandle(hwnd) : nullptr;

	if (!window || !manager) {
		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	double time = static_cast<double>(GetMessageTime()) / 1000.0;

	switch (message) {
		case WM_DESTROY: {
			window->should_close_ = true;

			if (manager->DestroyEvent) {
				manager->DestroyEvent(window);
			}
		} break;
		case WM_SIZE: {
			window->clientx = LOWORD(lparam);
			window->clienty = HIWORD(lparam);

			RECT rect;
			GetWindowRect(hwnd, &rect);
			window->sizex = rect.right - rect.left;
			window->sizey = rect.bottom - rect.top;

			switch (wparam) {
				case SIZE_MAXIMIZED: {
					window->state_ = State::maximized;
				} break;
				case SIZE_MINIMIZED: {
					window->state_ = State::minimized;
				} break;
				case SIZE_RESTORED: {
					window->state_ = State::normal;
				} break;
			}

			if (manager->ResizeEvent) {
				manager->ResizeEvent(window, window->clientx, window->clienty);
			}
		} break;
		case WM_MOVE: {
			window->posx = LOWORD(lparam);
			window->posy = HIWORD(lparam);

			if (manager->MoveEvent) {
				manager->MoveEvent(window, window->posx, window->posy);
			}
		} break;
		case WM_ACTIVATE: {
			if (LOWORD(wparam) == WA_INACTIVE) {
				if (manager->ActivateEvent) {
					manager->ActivateEvent(window, false);
				}
				window->active_ = false;
			} else {
				if (manager->ActivateEvent) {
					manager->ActivateEvent(window, true);
				}
				window->active_ = true;
			}
		} break;
		case WM_INPUTLANGCHANGE: {
			manager->keyboard_layout_ = (HKL)lparam;
		} break;
		case WM_INPUT: {
			if(!window->active_) {
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

					int tiny_window_key = convert_key(virtual_key, make_code, flags & (RI_KEY_E1 | RI_KEY_E0));

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
						if ((ret = ::ToUnicodeEx(virtual_key, raw.data.keyboard.MakeCode, state, utf16, 2, 0, manager->keyboard_layout_)) > 0) {
							ret = ::WideCharToMultiByte(CP_UTF8, 0, utf16, ret, utf8, sizeof(utf8), NULL, NULL);
							if (0 <= ret && ret < sizeof(utf8) / sizeof(utf8[0])) {
								utf8[ret] = '\0';
							}
						}
					}

					if (keydown && manager->KeyDownEvent) {
						manager->KeyDownEvent(window, tiny_window_key, repeat, utf8, time);
					}
					if (!keydown && manager->KeyUpEvent) {
						manager->KeyUpEvent(window, tiny_window_key, time);
					}
				} break;
			}
		} break;
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN: {
			if(!window->active_) {
				break;
			}
			
			int x = static_cast<int>(GET_X_LPARAM(lparam));
			int y = static_cast<int>(GET_Y_LPARAM(lparam));

			switch (message) {
				case WM_LBUTTONDOWN: {
					if (manager->ButtonDownEvent) {
						manager->ButtonDownEvent(window, WTK_BTN_LEFT, x, y, time);
					}
				} break;
				case WM_MBUTTONDOWN: {
					if (manager->ButtonDownEvent) {
						manager->ButtonDownEvent(window, WTK_BTN_MIDDLE, x, y, time);
					}
				} break;
				case WM_RBUTTONDOWN: {
					if (manager->ButtonDownEvent) {
						manager->ButtonDownEvent(window, WTK_BTN_RIGHT, x, y, time);
					}
				} break;
			}
		} break;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP: {
			if(!window->active_) {
				break;
			}
			
			int x = static_cast<int>(GET_X_LPARAM(lparam));
			int y = static_cast<int>(GET_Y_LPARAM(lparam));

			switch (message) {
				case WM_LBUTTONUP: {
					if (manager->ButtonUpEvent) {
						manager->ButtonUpEvent(window, WTK_BTN_LEFT, x, y, time);
					}
				} break;
				case WM_MBUTTONUP: {
					if (manager->ButtonUpEvent) {
						manager->ButtonUpEvent(window, WTK_BTN_MIDDLE, x, y, time);
					}
				} break;
				case WM_RBUTTONUP: {
					if (manager->ButtonUpEvent) {
						manager->ButtonUpEvent(window, WTK_BTN_RIGHT, x, y, time);
					}
				} break;
			}
		} break;
		case WM_MOUSEMOVE: {
			if(!window->active_) {
				break;
			}
			
			int x = static_cast<int>(GET_X_LPARAM(lparam));
			int y = static_cast<int>(GET_Y_LPARAM(lparam));

			if (manager->MouseEvent) {
				manager->MouseEvent(window, x, y, time);
			}
		} break;
		case WM_MOUSEWHEEL: {
			if(!window->active_) {
				break;
			}
			
			int delta = GET_WHEEL_DELTA_WPARAM(wparam);

			if (manager->WheelEvent) {
				manager->WheelEvent(window, 0, delta / WHEEL_DELTA, time);
			}
		} break;
	}

	return DefWindowProc(hwnd, message, wparam, lparam);
}
