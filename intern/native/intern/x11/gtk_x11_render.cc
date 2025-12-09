#include "gtk_x11_render.hh"
#include "gtk_x11_window.hh"
#include "gtk_x11_window_manager.hh"

#include <stdio.h>

static GLXContext SharedContextHandle = {};
static int SharedContextCounter = {};

GTKRenderXGL::GTKRenderXGL(GTKWindowX11 *window) : GTKRenderInterface(window), context(), pixel_config() {
	this->visual_info = NULL;
}

GTKRenderXGL::~GTKRenderXGL() {
	if (this->context) {
		if (SharedContextHandle != this->context || SharedContextCounter == 1) {
			if (--SharedContextCounter == 0) {
				SharedContextHandle = {};
			}
		}
	}
	
	MEM_SAFE_FREE(this->visual_attribs);
}

bool GTKRenderXGL::Create(RenderSetting setting) {
	GTKWindowX11 *window = static_cast<GTKWindowX11 *>(this->GetWindowInterface());
	GTKManagerX11 *manager = static_cast<GTKManagerX11 *>(window->GetManagerInterface());
	
	int i = 0;
	this->visual_attribs = static_cast<int *>(MEM_mallocN(sizeof(int) * 16, "VisualAttributes"));
	this->visual_attribs[i++] = GLX_RGBA;
	this->visual_attribs[i++] = GLX_DOUBLEBUFFER;
	this->visual_attribs[i++] = GLX_RED_SIZE;
	this->visual_attribs[i++] = setting.color_bits;
	this->visual_attribs[i++] = GLX_GREEN_SIZE;
	this->visual_attribs[i++] = setting.color_bits;
	this->visual_attribs[i++] = GLX_BLUE_SIZE;
	this->visual_attribs[i++] = setting.color_bits;
	this->visual_attribs[i++] = GLX_ALPHA_SIZE;
	this->visual_attribs[i++] = setting.color_bits;
	this->visual_attribs[i++] = GLX_DEPTH_SIZE;
	this->visual_attribs[i++] = setting.depth_bits;
	this->visual_attribs[i++] = GLX_STENCIL_SIZE;
	this->visual_attribs[i++] = setting.stencil_bits;
	this->visual_attribs[i++] = None;
	
	this->visual_info = glXChooseVisual(manager->display, XDefaultScreen(manager->display), this->visual_attribs);
	
	if (!this->visual_info) {
		return false;
	}
	
	GLXContext dummyContext = glXCreateContext(manager->display, this->visual_info, 0, true);
	glXMakeCurrent(manager->display, window->GetHandle(), dummyContext);
	
	if (!manager->XExtensionsInit()) {
		return false;
	}
	
	if (!XPixelConfigInit(setting)) {
		return false;
	}
	
	int attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, setting.major,
		GLX_CONTEXT_MINOR_VERSION_ARB, setting.minor,
		GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		None
	};
	
	this->context = manager->glxCreateContextAttribsARB(manager->display, this->pixel_config, SharedContextHandle, true, attribs);
	
	if (!this->context) {
		return false;
	}
	
	SharedContextCounter++;
	
	if (!SharedContextHandle) {
		SharedContextHandle = this->context;
	}
	
	glXMakeCurrent(manager->display, window->GetHandle(), this->context);
	
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return false;
	}
	
	return true;
}

bool GTKRenderXGL::MakeCurrent(void) {
	GTKWindowX11 *window = static_cast<GTKWindowX11 *>(this->GetWindowInterface());
	GTKManagerX11 *manager = static_cast<GTKManagerX11 *>(window->GetManagerInterface());
	
	glXMakeCurrent(manager->display, window->GetHandle(), this->context);
}

bool GTKRenderXGL::SwapBuffers(void) {
	GTKWindowX11 *window = static_cast<GTKWindowX11 *>(this->GetWindowInterface());
	GTKManagerX11 *manager = static_cast<GTKManagerX11 *>(window->GetManagerInterface());
	
	glXSwapBuffers(manager->display, window->GetHandle());
}

bool GTKRenderXGL::SwapInterval(int interval) {
	return false;
}

bool GTKRenderXGL::XPixelConfigInit(RenderSetting setting) {
	GTKWindowX11 *window = static_cast<GTKWindowX11 *>(this->GetWindowInterface());
	GTKManagerX11 *manager = static_cast<GTKManagerX11 *>(window->GetManagerInterface());
	
	int attribs[64], nattribs = 0;
		
	attribs[nattribs++] = GLX_X_RENDERABLE;
	attribs[nattribs++] = true;
	attribs[nattribs++] = GLX_RENDER_TYPE;
	attribs[nattribs++] = GLX_RGBA_BIT;
	attribs[nattribs++] = GLX_DRAWABLE_TYPE;
	attribs[nattribs++] = GLX_WINDOW_BIT;
	attribs[nattribs++] = GLX_X_VISUAL_TYPE;
	attribs[nattribs++] = GLX_TRUE_COLOR;
	attribs[nattribs++] = GLX_DOUBLEBUFFER;
	attribs[nattribs++] = true;
	
	attribs[nattribs++] = GLX_RED_SIZE;
	attribs[nattribs++] = setting.color_bits;
	attribs[nattribs++] = GLX_GREEN_SIZE;
	attribs[nattribs++] = setting.color_bits;
	attribs[nattribs++] = GLX_BLUE_SIZE;
	attribs[nattribs++] = setting.color_bits;
	attribs[nattribs++] = GLX_ALPHA_SIZE;
	attribs[nattribs++] = setting.color_bits;
	
	attribs[nattribs++] = GLX_DEPTH_SIZE;
	attribs[nattribs++] = setting.depth_bits;
	
	attribs[nattribs++] = GLX_STENCIL_SIZE;
	attribs[nattribs++] = setting.stencil_bits;
	attribs[nattribs++] = None;
	
	int count, best = 0;
	
	int screen = XDefaultScreen(manager->display);
	GLXFBConfig* configs = manager->glxChooseFBConfig(manager->display, screen, attribs, &count);
	
	if (!count) {
		return false;
	}

	for (int index = 0; index < count; index++) {
		XVisualInfo* visualInfo = glXGetVisualFromFBConfig(manager->display, configs[index]);

		if (visualInfo) {
			int samples, sampleBuffer;
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_SAMPLE_BUFFERS, &sampleBuffer);
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_SAMPLES, &samples);

			int bufferSize = 0;
			int doubleBuffer = 0;
			int numAuxBuffers = 0;
			int redSize = 0;
			int greenSize = 0;
			int blueSize = 0;
			int alphaSize = 0;
			int depthSize = 0;
			int stencilSize = 0;

			glXGetFBConfigAttrib(manager->display, configs[index], GLX_BUFFER_SIZE, &bufferSize);
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_DOUBLEBUFFER, &doubleBuffer);
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_AUX_BUFFERS, &numAuxBuffers);
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_RED_SIZE, &redSize);
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_GREEN_SIZE, &greenSize);
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_BLUE_SIZE, &blueSize);
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_ALPHA_SIZE, &alphaSize);
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_DEPTH_SIZE, &depthSize);
			glXGetFBConfigAttrib(manager->display, configs[index], GLX_STENCIL_SIZE, &stencilSize);

			if (sampleBuffer && samples > -1) {
				best = index;
			}
		}

		XFree(visualInfo);
	}
	
	this->pixel_config = configs[0];
	XFree(configs);
	
	return true;
}
