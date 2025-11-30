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

bool GTKRenderWGL::InitPixelFormat(GTKManagerWin32 *manager, GTKWindowWin32 *window, RenderSetting setting) {
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
