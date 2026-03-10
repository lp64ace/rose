#include "gtk_win32_render.hh"
#include "gtk_win32_window.hh"
#include "gtk_win32_window_manager.hh"

static HGLRC SharedContextHandle = nullptr;
static INT SharedContextCounter = 0;

GTKRenderWGL::GTKRenderWGL(GTKWindowWin32 *window) : GTKRenderInterface(window) {
	this->context = NULL;
}

GTKRenderWGL ::~GTKRenderWGL() {
	if (this->context) {
		if (SharedContextHandle != this->context || SharedContextCounter == 1) {
			if (--SharedContextCounter == 0) {
				SharedContextHandle = NULL;
			}
			wglDeleteContext(this->context);
		}
	}
}

static int WeightDummyPixelFormat(PIXELFORMATDESCRIPTOR *pfd, PIXELFORMATDESCRIPTOR *preferredPFD) {
	int weight = 0;

	/* Assume desktop color depth is 32 bits per pixel */

	/* cull unusable pixel formats */
	/* if no formats can be found, can we determine why it was rejected? */
	if (!(pfd->dwFlags & PFD_SUPPORT_OPENGL) || !(pfd->dwFlags & PFD_DRAW_TO_WINDOW) || !(pfd->dwFlags & PFD_DOUBLEBUFFER) || !(pfd->iPixelType == PFD_TYPE_RGBA) || (pfd->cColorBits > 32) || (pfd->dwFlags & PFD_GENERIC_FORMAT)) {
		return 0;
	}

	weight = 1; /* it's usable */

	weight += pfd->cColorBits - 8;

	if (preferredPFD->cAlphaBits > 0 && pfd->cAlphaBits > 0) {
		weight++;
	}

	return weight;
}


static HWND CloneWindow(HWND hWnd, LPVOID lpParam) {
	WCHAR lpClassName[100] = L"";
	WCHAR lpWindowName[100] = L"";

	GetClassNameW(hWnd, lpClassName, ARRAYSIZE(lpClassName));
	GetWindowTextW(hWnd, lpWindowName, ARRAYSIZE(lpWindowName));

	RECT rect;
	GetWindowRect(hWnd, &rect);

	/* clang-format off */

	HWND hwndCloned = CreateWindowExW(
		GetWindowLong(hWnd, GWL_EXSTYLE),
		lpClassName,
		lpWindowName,
		GetWindowLong(hWnd, GWL_STYLE),
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		(HWND)GetWindowLongPtr(hWnd, GWLP_HWNDPARENT),
		GetMenu(hWnd),
		(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
		lpParam
	);

	/* clang-format on */

	return hwndCloned;
}

/*
 * A modification of Ron Fosner's replacement for ChoosePixelFormat
 * returns 0 on error, else returns the pixel format number to be used
 */
static int ChooseDummyPixelFormatLegacy(HDC hDC, PIXELFORMATDESCRIPTOR *preferredPFD) {
	int iPixelFormat = 0;
	int weight = 0;

	int iStereoPixelFormat = 0;
	int stereoWeight = 0;

	/* choose a pixel format using the useless Windows function in case we come up empty handed */
	int iLastResortPixelFormat = ::ChoosePixelFormat(hDC, preferredPFD);
	int lastPFD = ::DescribePixelFormat(hDC, 1, sizeof(PIXELFORMATDESCRIPTOR), nullptr);

	for (int i = 1; i <= lastPFD; i++) {
		PIXELFORMATDESCRIPTOR pfd;
		int check = ::DescribePixelFormat(hDC, i, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

		int w = WeightDummyPixelFormat(&pfd, preferredPFD);

		if (w > weight) {
			weight = w;
			iPixelFormat = i;
		}

		if (w > stereoWeight && (preferredPFD->dwFlags & pfd.dwFlags & PFD_STEREO)) {
			stereoWeight = w;
			iStereoPixelFormat = i;
		}
	}

	/* choose any available stereo format over a non-stereo format */
	if (iStereoPixelFormat != 0) {
		iPixelFormat = iStereoPixelFormat;
	}

	if (iPixelFormat == 0) {
		fprintf(stderr, "Warning! Using result of ChoosePixelFormat.\n");
		iPixelFormat = iLastResortPixelFormat;
	}

	return iPixelFormat;
}

struct DummyContextWGL {
	HWND dummyHWND = nullptr;

	HDC dummyHDC = nullptr;
	HGLRC dummyHGLRC = nullptr;

	HDC prevHDC = nullptr;
	HGLRC prevHGLRC = nullptr;

	int dummyPixelFormat = 0;

	PIXELFORMATDESCRIPTOR preferredPFD;

	bool has_WGL_ARB_pixel_format = false;
	bool has_WGL_ARB_create_context = false;
	bool has_WGL_ARB_create_context_profile = false;
	bool has_WGL_ARB_create_context_robustness = false;

	DummyContextWGL(GTKManagerWin32 *manager, HDC hDC, HWND hWnd, bool stereoVisual, bool needAlpha) {
		prevHDC = ::wglGetCurrentDC();
		prevHGLRC = ::wglGetCurrentContext();

		/* clang-format off */

		preferredPFD = {
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			(DWORD)(PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | (stereoVisual ? PFD_STEREO : 0)),
			PFD_TYPE_RGBA,
			(BYTE)(needAlpha ? 32 : 24),
			0,
			0,
			0,
			0,
			0,
			0,						   
			(BYTE)(needAlpha ? 8 : 0), 
			0,						   
			0,						   
			0,
			0,
			0,
			0,				
			0,				
			0,				
			0,				
			PFD_MAIN_PLANE, 
			0,				
			0,
			0,
			0
		};

		/* clang-format on */

		dummyPixelFormat = ChooseDummyPixelFormatLegacy(hDC, &preferredPFD);

		if (dummyPixelFormat == 0) {
			return;
		}

		PIXELFORMATDESCRIPTOR chosenPFD;
		if (!::DescribePixelFormat(hDC, dummyPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &chosenPFD)) {
			return;
		}

		if (hWnd) {
			dummyHWND = CloneWindow(hWnd, nullptr);

			if (dummyHWND == nullptr) {
				return;
			}

			dummyHDC = GetDC(dummyHWND);
		}

		if (!(dummyHDC != nullptr)) {
			return;
		}

		if (!::SetPixelFormat(dummyHDC, dummyPixelFormat, &chosenPFD)) {
			return;
		}

		dummyHGLRC = ::wglCreateContext(dummyHDC);

		if (!(dummyHGLRC != nullptr)) {
			return;
		}

		if (!(::wglMakeCurrent(dummyHDC, dummyHGLRC))) {
			return;
		}

		if (glewInit() != GLEW_OK) {
			return;
		}

		has_WGL_ARB_pixel_format = glewIsSupported("WGL_ARB_pixel_format");
		has_WGL_ARB_create_context = glewIsSupported("WGL_ARB_create_context");
		has_WGL_ARB_create_context_profile = glewIsSupported("WGL_ARB_create_context_profile");
		has_WGL_ARB_create_context_robustness = glewIsSupported("WGL_ARB_create_context_robustness");
	}

	~DummyContextWGL() {
		::wglMakeCurrent(prevHDC, prevHGLRC);

		if (dummyHGLRC != nullptr) {
			::wglDeleteContext(dummyHGLRC);
		}

		if (dummyHWND != nullptr) {
			if (dummyHDC != nullptr) {
				::ReleaseDC(dummyHWND, dummyHDC);
			}

			::DestroyWindow(dummyHWND);
		}
	}
};

bool GTKRenderWGL::InitPixelFormat(GTKManagerWin32 *manager, GTKWindowWin32 *window, RenderSetting setting) {
	DummyContextWGL dummy(manager, window->GetDeviceHandle(), window->GetHandle(), setting.srgb, true);

	/* clang-format off */

	unsigned int count = WGL_NUMBER_PIXEL_FORMATS_ARB;
	int index = 0;
	int attribs[] = {
		WGL_SUPPORT_OPENGL_ARB, 1,
		WGL_DRAW_TO_WINDOW_ARB, 1,
		WGL_DOUBLE_BUFFER_ARB, 1,
		WGL_RED_BITS_ARB, setting.color_bits,
		WGL_GREEN_BITS_ARB, setting.color_bits,
		WGL_BLUE_BITS_ARB, setting.color_bits,
		WGL_ALPHA_BITS_ARB, setting.color_bits,
		WGL_DEPTH_BITS_ARB, setting.depth_bits,
		WGL_STENCIL_BITS_ARB, setting.stencil_bits,
		WGL_ACCUM_RED_BITS_ARB, setting.accum_bits,
		WGL_ACCUM_GREEN_BITS_ARB, setting.accum_bits,
		WGL_ACCUM_BLUE_BITS_ARB, setting.accum_bits,
		WGL_ACCUM_ALPHA_BITS_ARB, setting.accum_bits,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB
	};

	/* clang-format on */

	std::vector<int> attribList;
	attribList.assign(attribs, attribs + std::size(attribs));

	PIXELFORMATDESCRIPTOR pfd = {};
	if (wglChoosePixelFormatARB != nullptr) {
		if (setting.srgb) {
			attribList.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB);
		}

		attribList.push_back(0);  // needs a 0 to notify as the end of the list.
		wglChoosePixelFormatARB(window->GetDeviceHandle(), &attribList[0], nullptr, 1, &index, &count);
		if (DescribePixelFormat(window->GetDeviceHandle(), index, sizeof(pfd), &pfd)) {
			SetPixelFormat(window->GetDeviceHandle(), index, &pfd);
			return true;
		}
	}
	if (wglChoosePixelFormatEXT != nullptr) {
		if (setting.srgb) {
			attribList.push_back(WGL_FRAMEBUFFER_SRGB_CAPABLE_EXT);
		}

		attribList.push_back(0);
		wglChoosePixelFormatEXT(window->GetDeviceHandle(), &attribList[0], nullptr, 1, &index, &count);
		if (DescribePixelFormat(window->GetDeviceHandle(), index, sizeof(pfd), &pfd)) {
			SetPixelFormat(window->GetDeviceHandle(), index, &pfd);
			return true;
		}
	}
	FormatSetting format = {};
	index = manager->GetLegacyPFD(&format, window->GetDeviceHandle());
	if (!DescribePixelFormat(window->GetDeviceHandle(), index, sizeof(pfd), &pfd)) {
		return false;
	}
	SetPixelFormat(window->GetDeviceHandle(), index, &pfd);
	return true;
}

bool GTKRenderWGL::Create(RenderSetting setting) {
	GTKWindowWin32 *window = static_cast<GTKWindowWin32 *>(this->GetWindowInterface());
	GTKManagerWin32 *manager = static_cast<GTKManagerWin32 *>(window->GetManagerInterface());

	if (!InitPixelFormat(manager, window, setting)) {
		return false;
	}

	if (manager->wglCreateContextAttribsARB) {
		int attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB,
			setting.major,
			WGL_CONTEXT_MINOR_VERSION_ARB,
			setting.minor,
			0,
		};

		this->context = wglCreateContext(window->GetDeviceHandle());

		if (!this->context) {
			return false;
		}
	}
	else {
		this->context = wglCreateContext(window->GetDeviceHandle());
	}

	wglMakeCurrent(window->GetDeviceHandle(), this->context);
	if (manager->wglSwapIntervalEXT) {
		manager->wglSwapIntervalEXT(0);
	}

	GLenum err = glewInit();
	if (GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return false;
	}

	SharedContextCounter++;

	if (!SharedContextHandle) {
		SharedContextHandle = this->context;
	}
	else {
		wglShareLists(SharedContextHandle, this->context);
	}

	return true;
}

bool GTKRenderWGL::MakeCurrent(void) {
	GTKWindowWin32 *window = static_cast<GTKWindowWin32 *>(this->GetWindowInterface());

	return ::wglMakeCurrent(window->GetDeviceHandle(), this->context) != FALSE;
}

bool GTKRenderWGL::SwapBuffers(void) {
	GTKWindowWin32 *window = static_cast<GTKWindowWin32 *>(this->GetWindowInterface());

	return ::SwapBuffers(window->GetDeviceHandle()) != FALSE;
}

bool GTKRenderWGL::SwapInterval(int interval) {
	GTKWindowWin32 *window = static_cast<GTKWindowWin32 *>(this->GetWindowInterface());
	GTKManagerWin32 *manager = static_cast<GTKManagerWin32 *>(window->GetManagerInterface());

	if (manager->wglSwapIntervalEXT) {
		HDC PrevDeviceHandle = wglGetCurrentDC();
		HGLRC PrevContextHandle = wglGetCurrentContext();

		::wglMakeCurrent(window->GetDeviceHandle(), this->context);
		manager->wglSwapIntervalEXT(interval);
		::wglMakeCurrent(PrevDeviceHandle, PrevContextHandle);

		return true;
	}

	return false;
}
