#include "oswin.hh"
#include "oswin.h"

#include <assert.h>
#include <stdio.h>
#include <limits.h>

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

static int convert_key(const KeySym x11_key);
static int convert_key_ex(const KeySym x11_key, XkbDescPtr xkb_descr, const KeyCode x11_code);

tWindowManager::tWindowManager() {
	this->start_ = std::chrono::high_resolution_clock::now();
}

tWindowManager::~tWindowManager() {
	ClearKeyboardDescription();
	ClearExtensions();
	
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
	if (this->xim_) {
		XCloseIM(xim_);
	}
#endif
	if(this->display_) {
		XCloseDisplay(this->display_);
	}
	
	for (FormatSetting *format : formats_) {
		MEM_delete<FormatSetting>(format);
	}
	formats_.clear();
}

/* -------------------------------------------------------------------- */
/** \name [WindowManager] Linux Public Methods
 * \{ */

tWindowManager *tWindowManager::Init(const char *application) {
	tWindowManager *manager = new tWindowManager;

	if(!(manager->display_ = XOpenDisplay(NULL))) {
		delete manager;
		return nullptr;
	}
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
	if(!manager->InitXIM()) {
		// delete manager;
		// return nullptr;
	}
#endif
	if(!manager->InitKeyboardDescription()) {
		delete manager;
		return nullptr;
	}

	return manager;
}

bool tWindowManager::HasEventsWaiting() {
	if (XEventsQueued(this->display_, QueuedAfterReading)) {
		return true;
	}
	return false;
}
void tWindowManager::Poll() {
	XEvent evt;
	while (XEventsQueued(this->display_, QueuedAfterReading)) {
		XNextEvent(this->display_, &evt);
		
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
		/**
         *                                      ::
         *                                      =-
         *                                      ==
         *                                      ==
         *                                      ==
         *                                      -=
         *                                      -=
         *                                      :=
         *                                      :=
         *                                      :=
         *                                      :=
         *                                      :+
         *                                      :+
         *                                      :+ =%%%.
         *                                      =*%%%%%%*
         *                                      **-%*%%%%
         *                                     =*-::--:=
         *                                  .-::=*:-:=
         *                               .=::::=+*-:
         *                               :-:::::+::::
         *                              ::::::::*:::=
         *                              -:::::::*::.-
         *                              +::-:::-+-:::	- I was trying for 4 hours to add multi-language support functionality, 
         *                              -::-:::=++:::	that said, I had forgot to install a second language for the keyboard, 
         *                              :::-:::***:::	and I was also trying to test that through PuTTY and VcXsrv which... 
         *                              :::-::::-::.:	IT DOES NOT SUPPORT MULTI-LANGUAGE (either that or I am retarded)!
         *                              ::::::::-::=:.
         *                             .:::########::
         *                             ..::*********=
         *                             ===*****#***#-
         *                             =-+*********#
         *                               :****#****#
         *                                **********
         *                               :****=+***+
         *                               =****.+***:
         *                               ****# ****.
         *                               ****  ***#
         *                              :****  #***
         *                              :#**. .***:
         *                               %%#   %#*
         *                               +%%- :%%:
         *                                .%#  %-
		 */
		if(evt.type == FocusIn || evt.type == KeyPress) {
			if(!this->xim_) {
				this->InitXIM();
			}
			if(this->xim_) {
				tWindow *window = this->GetWindowByHandle(evt.xany.window);
				if(window && !window->xic_ && window->InitXIC(this)) {
					if (evt.type == KeyPress) {
						XSetICFocus(window->xic_);
					}
				}
			}
		}
#endif
		EventProcedure(&evt);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name [WindowManager] Linux Private Methods
 * \{ */

tWindow *tWindowManager::GetWindowByHandle(Window handle) const {
	for (auto itr = this->windows_.begin(); itr != this->windows_.end(); itr++) {
		if ((*itr)->window_handle_ == handle) {
			return *itr;
		}
	}
	return nullptr;
}

bool tWindowManager::InitExtensions() {
	if(!(glxCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const unsigned char*)"glXCreateContextAttribsARB"))) {
		return false;
	}
	if(!(glxMakeContextCurrent = (PFNGLXMAKECONTEXTCURRENTPROC)glXGetProcAddress((const unsigned char*)"glXMakeContextCurrent"))) {
		return false;
	}
	if(!(glxSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const unsigned char*)"glXSwapIntervalEXT"))) {
		// return false;
	}
	if(!(glxChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)glXGetProcAddress((const unsigned char*)"glXChooseFBConfig"))) {
		return false;
	}
	return true;
}

bool tWindowManager::InitKeyboardDescription() {
	/* Use detectable auto-repeat, mac and windows also do this. */
	int use_xkb;
	int xkb_opcode, xkb_event, xkb_error;
	int xkb_major = XkbMajorVersion, xkb_minor = XkbMinorVersion;
	use_xkb = XkbQueryExtension(this->display_, &xkb_opcode, &xkb_event, &xkb_error, &xkb_major, &xkb_minor);
	if (use_xkb) {
		XkbSetDetectableAutoRepeat(this->display_, true, nullptr);
		
		this->xkb_descr_ = XkbGetMap(this->display_, 0, XkbUseCoreKbd);
		if(this->xkb_descr_) {
			XkbGetNames(this->display_, XkbKeyNamesMask, this->xkb_descr_);
			XkbGetControls(this->display_, XkbPerKeyRepeatMask | XkbRepeatKeysMask, this->xkb_descr_);
			return true;
		}
		return false;
	}
	return true;
}

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
static void destroyIMCallback(XIM xim, XPointer ptr, XPointer data) {
	if (ptr) {
		*(XIM *)ptr = nullptr;
	}
}

bool tWindowManager::InitXIM() {
	XSetLocaleModifiers("");
	
	this->xim_ = XOpenIM(
		this->display_,
		nullptr,
		(char *)"TinyWindow",
		(char *)"TinyWindow"
	);
	if (!this->xim_) {
		return false;
	}
	
	XIMCallback destroy;
	destroy.callback = (XIMProc)destroyIMCallback;
	destroy.client_data = (XPointer)&this->xim_;
	XSetIMValues(this->xim_, XNDestroyCallback, &destroy, nullptr);
	return true;
}
#endif
	
void tWindowManager::ClearExtensions() {
}

void tWindowManager::ClearKeyboardDescription() {
	if (this->xkb_descr_) {
		XkbFreeKeyboard(this->xkb_descr_, XkbAllComponentsMask, true);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name [Window] Linux Public Methods
 * \{ */

void tWindow::SetWindowStyle(Style style) {
	switch (style) {
		case Style::normal: {
			this->linux_decorations_ = LinuxDecorator::move | LinuxDecorator::close | LinuxDecorator::maximize | LinuxDecorator::minimize;
			long Hints[5] = {
				static_cast<long>(Hint::function | Hint::decorator),
				static_cast<long>(this->linux_decorations_),
				(1 << 2), 0, 0
			};
			XChangeProperty(this->display_, this->window_handle_, AtomHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)Hints, 5);
			XMapWindow(this->display_, this->window_handle_);
		} break;

		case Style::bare: {
			this->linux_decorations_ = LinuxDecorator::move;
			long Hints[5] = {
				static_cast<long>(Hint::function | Hint::decorator),
				static_cast<long>(this->linux_decorations_),
				(1 << 2), 0, 0
			};
			XChangeProperty(this->display_, this->window_handle_, AtomHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)Hints, 5);
			XMapWindow(this->display_, this->window_handle_);
		} break;

		case Style::popup: {
			this->linux_decorations_ = LinuxDecorator::move;
			long Hints[5] = {
				static_cast<long>(Hint::function | Hint::decorator),
				static_cast<long>(this->linux_decorations_),
				0, 0, 0
			};
			XChangeProperty(this->display_, this->window_handle_, AtomHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)Hints, 5);
			XMapWindow(this->display_, this->window_handle_);
		} break;
	}
}
void tWindow::SetWindowDecoration(Decorator decoration, bool add) {
	if ((decoration & Decorator::border) != Decorator::none) {
		// ?
	}
	if ((decoration & Decorator::titlebar) != Decorator::none) {
		// ?
	}
	if ((decoration & Decorator::icon) != Decorator::none) {
		// ?
	}
	if ((decoration & Decorator::destroy_btn) != Decorator::none) {
		SET_FLAG_FROM_TEST(this->linux_decorations_, add, LinuxDecorator::close);
	}
	if ((decoration & Decorator::minimize_btn) != Decorator::none) {
		SET_FLAG_FROM_TEST(this->linux_decorations_, add, LinuxDecorator::minimize);
	}
	if ((decoration & Decorator::maximize_btn) != Decorator::none) {
		SET_FLAG_FROM_TEST(this->linux_decorations_, add, LinuxDecorator::maximize);
	}
	if ((decoration & Decorator::sizeable) != Decorator::none) {
		// ?
	}
	
	long Hints[5] = {
		static_cast<long>(Hint::function | Hint::decorator),
		static_cast<long>(this->linux_decorations_),
		1, 0, 0
	};
	XChangeProperty(this->display_, this->window_handle_, AtomHints, XA_ATOM, 32, PropModeReplace, (unsigned char *)Hints, 5);
	XMapWindow(this->display_, this->window_handle_);
}
void tWindow::SetTitleBar(const char *title) {
	XStoreName(this->display_, this->window_handle_, title);
}
State tWindow::GetState() const {
	return this->state_;
}

void tWindow::Show() {
	XMapWindow(this->display_, this->window_handle_);
}
void tWindow::Hide() {
	XUnmapWindow(this->display_, this->window_handle_);
}

bool tWindow::MakeContextCurrent() {
	glXMakeCurrent(this->display_, this->window_handle_, this->rendering_handle_);
	return true;
}
bool tWindow::SetSwapInterval(int interval) {
	return false;
}
bool tWindow::GetSwapInterval(int *interval) {
	return false;
}
bool tWindow::SwapBuffers() {
	glXSwapBuffers(this->display_, this->window_handle_);
	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name [Window] Linux Private Methods
 * \{ */

tWindow::tWindow(WindowSetting settings) : settings(settings) {
}

tWindow::~tWindow() {
	if(this->visual_info_) {
		// Do we need to free this?
	}
	if(this->window_handle_) {
		XDestroyWindow(this->display_, this->window_handle_);
	}
	MEM_SAFE_FREE(this->visual_attribs_);
}

bool tWindow::Init(tWindowManager *manager) {
	if(!manager->display_) {
		return false;
	}
	
	this->display_ = manager->display_;
	
	{
		int i = 0;
		this->visual_attribs_ = static_cast<int *>(MEM_mallocN(sizeof(int) * 16, "VisualAttributes"));
		this->visual_attribs_[i++] = GLX_RGBA;
		this->visual_attribs_[i++] = GLX_DOUBLEBUFFER;
		this->visual_attribs_[i++] = GLX_RED_SIZE;
		this->visual_attribs_[i++] = settings.color_bits;
		this->visual_attribs_[i++] = GLX_GREEN_SIZE;
		this->visual_attribs_[i++] = settings.color_bits;
		this->visual_attribs_[i++] = GLX_BLUE_SIZE;
		this->visual_attribs_[i++] = settings.color_bits;
		this->visual_attribs_[i++] = GLX_ALPHA_SIZE;
		this->visual_attribs_[i++] = settings.color_bits;
		this->visual_attribs_[i++] = GLX_DEPTH_SIZE;
		this->visual_attribs_[i++] = settings.depth_bits;
		this->visual_attribs_[i++] = GLX_STENCIL_SIZE;
		this->visual_attribs_[i++] = settings.stencil_bits;
		this->visual_attribs_[i++] = None;
		
		this->visual_info_ = glXChooseVisual(this->display_, XDefaultScreen(this->display_), this->visual_attribs_);
	}
	
	if(!this->visual_info_) {
		return false;
	}
	
	this->linux_decorations_ = LinuxDecorator::close | LinuxDecorator::maximize | LinuxDecorator::minimize | LinuxDecorator::move;
	this->window_attributes_.colormap = XCreateColormap(this->display_, DefaultRootWindow(this->display_), this->visual_info_->visual, AllocNone);
	this->window_attributes_.event_mask |= ExposureMask;
	this->window_attributes_.event_mask |= KeyPressMask | KeyReleaseMask;
	this->window_attributes_.event_mask |= PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	this->window_attributes_.event_mask |= FocusIn | FocusOut | FocusChangeMask;
	this->window_attributes_.event_mask |= Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask;
	this->window_attributes_.event_mask |= VisibilityChangeMask | PropertyChangeMask | SubstructureNotifyMask | MotionNotify;
	
	this->window_handle_ = XCreateWindow(
		this->display_,
		XDefaultRootWindow(this->display_),
		0,
		0,
		settings.width,
		settings.height,
		0,
		this->visual_info_->depth,
		InputOutput,
		this->visual_info_->visual,
		CWColormap | CWEventMask,
		&this->window_attributes_
	);
	
	if(!this->window_handle_) {
		return false;
	}
	
	this->InitAtoms();
	
	XStoreName(this->display_, this->window_handle_, settings.name);
	
	unsigned long mask = 0;
		
	mask |= KeyPressMask | KeyReleaseMask;
	mask |= ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
	mask |= PointerMotionMask | EnterWindowMask | LeaveWindowMask;
	mask |= SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask | FocusChangeMask;
	mask |= ExposureMask;
	
	XSelectInput(this->display_, this->window_handle_, mask);
	XSetWMProtocols(this->display_, this->window_handle_, &this->AtomClose, 1);
	
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
	if(!this->InitXIC(manager)) {
		return false;
	}
#endif
	
	if (!(this->context_created_ = InitContext(manager))) {
		return false;
	}
	
	XWindowAttributes attributes;

	XGetWindowAttributes(this->display_, this->window_handle_, &attributes);
	this->posx = attributes.x;
	this->posy = attributes.y;
	// this->sizex = attributes.width;
	// this->sizey = attributes.height;
	this->clientx = attributes.width;
	this->clienty = attributes.height;
	
	if(manager->glxSwapIntervalEXT != nullptr) {
		manager->glxSwapIntervalEXT(this->display_, this->window_handle_, 0);
	}
	
	return true;
}
bool tWindow::InitContext(tWindowManager *manager) {
	GLXContext dummyContext = glXCreateContext(this->display_, this->visual_info_, 0, true);
	glXMakeCurrent(this->display_, this->window_handle_, dummyContext);
	
	if(!manager->InitExtensions()) {
		return false;
	}
	
	if(!InitPixelConfig(manager)) {
		return false;
	}
	
	int attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, settings.major,
		GLX_CONTEXT_MINOR_VERSION_ARB, settings.minor,
		GLX_CONTEXT_FLAGS_ARB,
		GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		None
	};
	
	this->rendering_handle_ = manager->glxCreateContextAttribsARB(this->display_, this->pixel_config_, NULL, true, attribs);
	
	if (!this->rendering_handle_) {
		return false;
	}
	
	glXMakeCurrent(this->display_, this->window_handle_, this->rendering_handle_);
	
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return false;
	}

	glXDestroyContext(this->display_, dummyContext);
	
	return this->context_created_ = true;
}
bool tWindow::InitPixelConfig(tWindowManager *manager) {
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
	attribs[nattribs++] = settings.color_bits;
	attribs[nattribs++] = GLX_GREEN_SIZE;
	attribs[nattribs++] = settings.color_bits;
	attribs[nattribs++] = GLX_BLUE_SIZE;
	attribs[nattribs++] = settings.color_bits;
	attribs[nattribs++] = GLX_ALPHA_SIZE;
	attribs[nattribs++] = settings.color_bits;
	
	attribs[nattribs++] = GLX_DEPTH_SIZE;
	attribs[nattribs++] = settings.depth_bits;
	
	attribs[nattribs++] = GLX_STENCIL_SIZE;
	attribs[nattribs++] = settings.stencil_bits;
	attribs[nattribs++] = None;
	
	int count, best = 0;
	
	int screen = XDefaultScreen(this->display_);
	GLXFBConfig* configs = manager->glxChooseFBConfig(this->display_, screen, attribs, &count);
	
	if(!count) {
		return false;
	}

	for (int index = 0; index < count; index++) {
		XVisualInfo* visualInfo = glXGetVisualFromFBConfig(this->display_, configs[index]);

		if (visualInfo) {
			int samples, sampleBuffer;
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_SAMPLE_BUFFERS, &sampleBuffer);
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_SAMPLES, &samples);

			int bufferSize = 0;
			int doubleBuffer = 0;
			int numAuxBuffers = 0;
			int redSize = 0;
			int greenSize = 0;
			int blueSize = 0;
			int alphaSize = 0;
			int depthSize = 0;
			int stencilSize = 0;

			glXGetFBConfigAttrib(this->display_, configs[index], GLX_BUFFER_SIZE, &bufferSize);
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_DOUBLEBUFFER, &doubleBuffer);
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_AUX_BUFFERS, &numAuxBuffers);
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_RED_SIZE, &redSize);
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_GREEN_SIZE, &greenSize);
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_BLUE_SIZE, &blueSize);
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_ALPHA_SIZE, &alphaSize);
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_DEPTH_SIZE, &depthSize);
			glXGetFBConfigAttrib(this->display_, configs[index], GLX_STENCIL_SIZE, &stencilSize);

			if (sampleBuffer && samples > -1) {
				best = index;
			}
		}

		XFree(visualInfo);
	}
	this->pixel_config_ = configs[0];
	XFree(configs);
	return true;
}
bool tWindow::InitAtoms() {
	assert(this->display_);
	this->AtomState = XInternAtom(this->display_, "_NET_WM_STATE", false);
	this->AtomFullScreen = XInternAtom(this->display_, "_NET_WM_STATE_FULLSCREEN", false);
	this->AtomMaxHorz = XInternAtom(this->display_, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
	this->AtomMaxVert = XInternAtom(this->display_, "_NET_WM_STATE_MAXIMIZED_VERT", false);
	this->AtomClose = XInternAtom(this->display_, "WM_DELETE_WINDOW", false);
	this->AtomHidden = XInternAtom(this->display_, "_NET_WM_STATE_HIDDEN", false);
	this->AtomActive = XInternAtom(this->display_, "_NET_ACTIVE_WINDOW", false);
	this->AtomDemandsAttention = XInternAtom(this->display_, "_NET_WM_STATE_DEMANDS_ATTENTION", false);
	this->AtomFocused = XInternAtom(this->display_, "_NET_WM_STATE_FOCUSED", false);
	this->AtomCardinal = XInternAtom(this->display_, "CARDINAL", false);
	this->AtomIcon = XInternAtom(this->display_, "_NET_WM_ICON", false);
	this->AtomHints = XInternAtom(this->display_, "_MOTIF_WM_HINTS", true);

	this->AtomWindowType = XInternAtom(this->display_, "_NET_WM_WINDOW_TYPE", false);
	this->AtomWindowTypeDesktop = XInternAtom(this->display_, "_NET_WM_WINDOW_TYPE_UTILITY", false);
	this->AtomWindowTypeSplash = XInternAtom(this->display_, "_NET_WM_WINDOW_TYPE_SPLASH", false);
	this->AtomWindowTypeNormal = XInternAtom(this->display_, "_NET_WM_WINDOW_TYPE_NORMAL", false);

	this->AtomAllowedActions = XInternAtom(this->display_, "_NET_WM_ALLOWED_ACTIONS", false);
	this->AtomActionResize = XInternAtom(this->display_, "WM_ACTION_RESIZE", false);
	this->AtomActionMinimize = XInternAtom(this->display_, "_WM_ACTION_MINIMIZE", false);
	this->AtomActionShade = XInternAtom(this->display_, "WM_ACTION_SHADE", false);
	this->AtomActionMaximizeHorz = XInternAtom(this->display_, "_WM_ACTION_MAXIMIZE_HORZ", false);
	this->AtomActionMaximizeVert = XInternAtom(this->display_, "_WM_ACTION_MAXIMIZE_VERT", false);
	this->AtomActionClose = XInternAtom(this->display_, "_WM_ACTION_CLOSE", false);

	this->AtomDesktopGeometry = XInternAtom(this->display_, "_NET_DESKTOP_GEOMETRY", false);
	return true;
}
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
static Bool destroyICCallback(XIC xic, XPointer ptr, XPointer data) {
	if (ptr) {
		*(XIC *)ptr = nullptr;
	}
	/* Ignored by X11. */
	return True;
}

bool tWindow::InitXIC(tWindowManager *manager) {
	XIM xim = manager->xim_;
	if (!xim) {
		return false;
	}
	XICCallback destroy;
	destroy.callback = (XICProc)destroyICCallback;
	destroy.client_data = (XPointer)&this->xic_;
	this->xic_ = XCreateIC(
		xim,
		XNClientWindow,
		this->window_handle_,
		XNFocusWindow,
		this->window_handle_,
		XNInputStyle,
		XIMPreeditNothing | XIMStatusNothing,
		XNResourceName,
		(this->settings.name) ? this->settings.name : "TinyWindow",
		XNResourceClass,
		(this->settings.name) ? this->settings.name : "TinyWindow",
		XNDestroyCallback,
		&destroy,
		nullptr
	);
	
	if(!this->xic_) {
		return false;
	}
	
	unsigned long mask = 0;
		
	mask |= KeyPressMask | KeyReleaseMask;
	mask |= ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
	mask |= PointerMotionMask | EnterWindowMask | LeaveWindowMask;
	mask |= SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask | FocusChangeMask;
	mask |= ExposureMask;
	
	unsigned long extra = 0;
	XGetICValues(this->xic_, XNFilterEvents, &extra, nullptr);
	XSelectInput(this->display_, this->window_handle_, mask | extra);
	return true;
}
#endif

/** \} */

static int convert_key(const KeySym x11_key) {
	int key = WTK_KEY_UNKOWN;
	
	if ((x11_key >= XK_A) && (x11_key <= XK_Z)) {
		key = (x11_key - XK_A + WTK_KEY_A);
	}
	else if ((x11_key >= XK_a) && (x11_key <= XK_z)) {
		key = (x11_key - XK_a + WTK_KEY_A);
	}
	else if ((x11_key >= XK_0) && (x11_key <= XK_9)) {
		key = (x11_key - XK_0 + WTK_KEY_0);
	}
	else if((x11_key >= XK_F1) && (x11_key <= XK_F24)) {
		key = (x11_key - XK_F1 + WTK_KEY_F1);
	}
	else {
		
#define ASSOCIATE(store, what, to) case what: store = to; break;
		
		switch(x11_key) {
			ASSOCIATE(key, XK_BackSpace, WTK_KEY_BACKSPACE);
			ASSOCIATE(key, XK_Tab, WTK_KEY_TAB);
			ASSOCIATE(key, XK_ISO_Left_Tab, WTK_KEY_TAB);
			ASSOCIATE(key, XK_Return, WTK_KEY_ENTER);
			ASSOCIATE(key, XK_Escape, WTK_KEY_ESC);
			ASSOCIATE(key, XK_space, WTK_KEY_SPACE);
			
			ASSOCIATE(key, XK_semicolon, WTK_KEY_SEMICOLON);
			ASSOCIATE(key, XK_period, WTK_KEY_PERIOD);
			ASSOCIATE(key, XK_comma, WTK_KEY_COMMA);
			ASSOCIATE(key, XK_quoteright, WTK_KEY_QUOTE);
			ASSOCIATE(key, XK_quoteleft, WTK_KEY_ACCENTGRAVE);
			ASSOCIATE(key, XK_minus, WTK_KEY_MINUS);
			ASSOCIATE(key, XK_plus, WTK_KEY_PLUS);
			ASSOCIATE(key, XK_slash, WTK_KEY_SLASH);
			ASSOCIATE(key, XK_backslash, WTK_KEY_BACKSLASH);
			ASSOCIATE(key, XK_equal, WTK_KEY_EQUAL);
			ASSOCIATE(key, XK_bracketleft, WTK_KEY_LEFT_BRACKET);
			ASSOCIATE(key, XK_bracketright, WTK_KEY_RIGHT_BRACKET);
			ASSOCIATE(key, XK_Pause, WTK_KEY_PAUSE);
			
			ASSOCIATE(key, XK_Shift_L, WTK_KEY_LEFT_SHIFT);
			ASSOCIATE(key, XK_Shift_R, WTK_KEY_RIGHT_SHIFT);
			ASSOCIATE(key, XK_Control_L, WTK_KEY_LEFT_CTRL);
			ASSOCIATE(key, XK_Control_R, WTK_KEY_RIGHT_CTRL);
			ASSOCIATE(key, XK_Alt_L, WTK_KEY_LEFT_ALT);
			ASSOCIATE(key, XK_Alt_R, WTK_KEY_RIGHT_ALT);
			ASSOCIATE(key, XK_Super_L, WTK_KEY_LEFT_OS);
			ASSOCIATE(key, XK_Super_R, WTK_KEY_RIGHT_OS);
			
			ASSOCIATE(key, XK_Insert, WTK_KEY_INSERT);
			ASSOCIATE(key, XK_Delete, WTK_KEY_DELETE);
			ASSOCIATE(key, XK_Home, WTK_KEY_HOME);
			ASSOCIATE(key, XK_End, WTK_KEY_END);
			ASSOCIATE(key, XK_Page_Up, WTK_KEY_PAGEUP);
			ASSOCIATE(key, XK_Page_Down, WTK_KEY_PAGEDOWN);
			
			ASSOCIATE(key, XK_Left, WTK_KEY_LEFT);
			ASSOCIATE(key, XK_Right, WTK_KEY_RIGHT);
			ASSOCIATE(key, XK_Up, WTK_KEY_UP);
			ASSOCIATE(key, XK_Down, WTK_KEY_DOWN);
			
			ASSOCIATE(key, XK_Caps_Lock, WTK_KEY_CAPSLOCK);
			ASSOCIATE(key, XK_Scroll_Lock, WTK_KEY_SCRLOCK);
			ASSOCIATE(key, XK_Num_Lock, WTK_KEY_NUMLOCK);
			ASSOCIATE(key, XK_Menu, WTK_KEY_APP);
			
			ASSOCIATE(key, XK_KP_0, WTK_KEY_NUMPAD_0);
			ASSOCIATE(key, XK_KP_1, WTK_KEY_NUMPAD_1);
			ASSOCIATE(key, XK_KP_2, WTK_KEY_NUMPAD_2);
			ASSOCIATE(key, XK_KP_3, WTK_KEY_NUMPAD_3);
			ASSOCIATE(key, XK_KP_4, WTK_KEY_NUMPAD_4);
			ASSOCIATE(key, XK_KP_5, WTK_KEY_NUMPAD_5);
			ASSOCIATE(key, XK_KP_6, WTK_KEY_NUMPAD_6);
			ASSOCIATE(key, XK_KP_7, WTK_KEY_NUMPAD_7);
			ASSOCIATE(key, XK_KP_8, WTK_KEY_NUMPAD_8);
			ASSOCIATE(key, XK_KP_9, WTK_KEY_NUMPAD_9);
			ASSOCIATE(key, XK_KP_Decimal, WTK_KEY_NUMPAD_PERIOD);
			
			ASSOCIATE(key, XK_KP_Insert, WTK_KEY_NUMPAD_0);
			ASSOCIATE(key, XK_KP_End, WTK_KEY_NUMPAD_1);
			ASSOCIATE(key, XK_KP_Down, WTK_KEY_NUMPAD_2);
			ASSOCIATE(key, XK_KP_Page_Down, WTK_KEY_NUMPAD_3);
			ASSOCIATE(key, XK_KP_Left, WTK_KEY_NUMPAD_4);
			ASSOCIATE(key, XK_KP_Begin, WTK_KEY_NUMPAD_5);
			ASSOCIATE(key, XK_KP_Right, WTK_KEY_NUMPAD_6);
			ASSOCIATE(key, XK_KP_Home, WTK_KEY_NUMPAD_7);
			ASSOCIATE(key, XK_KP_Up, WTK_KEY_NUMPAD_8);
			ASSOCIATE(key, XK_KP_Page_Up, WTK_KEY_NUMPAD_9);
			ASSOCIATE(key, XK_KP_Delete, WTK_KEY_NUMPAD_PERIOD);
			
			ASSOCIATE(key, XK_KP_Enter, WTK_KEY_NUMPAD_ENTER);
			ASSOCIATE(key, XK_KP_Add, WTK_KEY_NUMPAD_PLUS);
			ASSOCIATE(key, XK_KP_Subtract, WTK_KEY_NUMPAD_MINUS);
			ASSOCIATE(key, XK_KP_Multiply, WTK_KEY_NUMPAD_ASTERISK);
			ASSOCIATE(key, XK_KP_Divide, WTK_KEY_NUMPAD_SLASH);
		}
		
#undef ASSOCIATE
	}
	
	return key;
}
static int convert_code(XkbDescPtr xkb_descr, const KeyCode x11_code) {
	if (x11_code >= xkb_descr->min_key_code && x11_code <= xkb_descr->max_key_code) {
		const char *id_str = xkb_descr->names->keys[x11_code].name;
		// fprintf(stdout, "[Linux/Key] %s\n", id_str);
	}
	return WTK_KEY_UNKOWN;
}
static int convert_key_ex(const KeySym x11_key, XkbDescPtr xkb_descr, const KeyCode x11_code) {
	int key = convert_key(x11_key);
	if (key == WTK_KEY_UNKOWN) {
		if (xkb_descr) {
			key = convert_code(xkb_descr, x11_code);
		}
	}
	return key;
}

}  // namespace rose::tiny_window

static unsigned char bit_is_on(const unsigned char *ptr, int bit) {
	return ptr[bit >> 3] & (1 << (bit & 7));
}

void rose::tiny_window::tWindowManager::EventProcedure(XEvent *evt) {
	rose::tiny_window::tWindow *window = this->GetWindowByHandle(evt->xany.window);
	
	if(!window) {
		return;
	}
	
	switch (evt->type) {
		case Expose: {
		} break;
		
		case DestroyNotify: {
			if (DestroyEvent != nullptr) {
				DestroyEvent(window);
			}
		} break;
		
		// when a request to resize the window is made either by dragging out the window or programmatically
		case ResizeRequest: {
			window->clientx = evt->xresizerequest.width;
			window->clienty = evt->xresizerequest.height;
			
			if (ResizeEvent) {
				ResizeEvent(window, window->clientx, window->clienty);
			}
		} break;

		// when a request to configure the window is made
		case ConfigureNotify: {
			// check if window was resized
			if (evt->xconfigure.width != window->clientx || evt->xconfigure.height != window->clienty) {
				window->clientx = evt->xconfigure.width;
				window->clienty = evt->xconfigure.height;
				
				if (ResizeEvent) {
					ResizeEvent(window, window->clientx, window->clienty);
				}
			}
			
			// check if window was moved
			if (evt->xconfigure.x != window->posx || evt->xconfigure.y != window->posy) {
				window->posx = evt->xconfigure.x;
				window->posy = evt->xconfigure.y;
				
				if (MoveEvent) {
					MoveEvent(window, window->posx, window->posy);
				}
			}
		} break;
		
		case PropertyNotify: {
			// this is needed in order to read from the windows WM_STATE Atomic
			// to determine if the property notify event was caused by a client
			// iconify Event(window, minimizing the window), a maximise event, a focus
			// event and an attention demand event. NOTE these should only be
			// for eventts that are not triggered programatically

			Atom type;
			int format;
			ulong numItems, bytesAfter;
			unsigned char *properties = nullptr;

			XGetWindowProperty(this->display_, evt->xproperty.window, window->AtomState, 0, LONG_MAX, false, AnyPropertyType, &type, &format, &numItems, &bytesAfter, &properties);

			if (properties && (format == 32)) {
				window->state_ = State::normal;
				// go through each property and match it to an existing Atomic state
				for (unsigned int itemIndex = 0; itemIndex < numItems; itemIndex++) {
					Atom currentProperty = ((long *)(properties))[itemIndex];

					if (currentProperty == window->AtomHidden) {
						window->state_ = State::minimized;
					}

					if (currentProperty == window->AtomMaxVert || currentProperty == window->AtomMaxVert) {
						window->state_ = State::maximized;
					}

					if (currentProperty == window->AtomFocused) {
						// window is now in focus. we can ignore this is as FocusIn/FocusOut does this anyway
					}

					if (currentProperty == window->AtomDemandsAttention) {
						// the window demands user attention
					}
				}
			}
		} break;
		
		case KeyPress:
		case KeyRelease: {
			XKeyEvent *xke = &(evt->xkey);

			char *utf8_buf = nullptr;
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			char utf8_array[16 * 6 + 5];
			int len = 1;
#else
			char utf8_array[4] = {'\0'};
#endif

			KeySym key_sym;
			/**
			 * XXX: Code below is kinda awfully convoluted... Issues are:
			 * - In keyboards like Latin ones, numbers need a 'Shift' to be accessed but key_sym
			 *   is unmodified (or anyone swapping the keys with `xmodmap`).
			 * - #XLookupKeysym seems to always use first defined key-map (see #47228), which generates
			 *   key-codes unusable by ghost_key_from_keysym for non-Latin-compatible key-maps.
			 *
			 * To address this, we:
			 * - Try to get a 'number' key_sym using #XLookupKeysym (with virtual shift modifier),
			 *   in a very restrictive set of cases.
			 * - Fallback to #XLookupString to get a key_sym from active user-defined key-map.
			 *
			 * Note that:
			 * - This effectively 'lock' main number keys to always output number events
			 *   (except when using alt-gr).
			 * - This enforces users to use an ASCII-compatible key-map with Blender -
			 *   but at least it gives predictable and consistent results.
			 *
			 * Also, note that nothing in XLib sources [1] makes it obvious why those two functions give
			 * different key_sym results.
			 *
			 * [1] http://cgit.freedesktop.org/xorg/lib/libX11/tree/src/KeyBind.c
			 */
			KeySym key_sym_str;
			/** 
			 * Mode_switch 'modifier' is `AltGr` - when this one or Shift are enabled, 
			 * we do not want to apply that 'forced number' hack.
			 */
			const unsigned int mode_switch_mask = XkbKeysymToModifiers(xke->display, XK_Mode_switch);
			const unsigned int number_hack_forbidden_kmods_mask = mode_switch_mask | ShiftMask;
			if((xke->keycode >= 10 && xke->keycode < 20) && ((xke->state & number_hack_forbidden_kmods_mask) == 0)) {
				key_sym = XLookupKeysym(xke, ShiftMask);
				if(!(XK_0 <= key_sym && key_sym <= XK_9)) {
					key_sym = XLookupKeysym(xke, 0);
				}
			}
			else {
				key_sym = XLookupKeysym(xke, 0);
			}
			
			char ascii;
			if (!XLookupString(xke, &ascii, 1, &key_sym_str, nullptr)) {
				ascii = '\0';
			}
			
			int mkey = convert_key_ex(key_sym, this->xkb_descr_, xke->keycode);
			switch (mkey) {
				case WTK_KEY_LEFT_ALT:
				case WTK_KEY_RIGHT_ALT:
				case WTK_KEY_LEFT_CTRL:
				case WTK_KEY_RIGHT_CTRL:
				case WTK_KEY_LEFT_SHIFT:
				case WTK_KEY_RIGHT_SHIFT:
				case WTK_KEY_LEFT_OS:
				case WTK_KEY_RIGHT_OS:
				case WTK_KEY_0:
				case WTK_KEY_1:
				case WTK_KEY_2:
				case WTK_KEY_3:
				case WTK_KEY_4:
				case WTK_KEY_5:
				case WTK_KEY_6:
				case WTK_KEY_7:
				case WTK_KEY_8:
				case WTK_KEY_9:
				case WTK_KEY_NUMPAD_0:
				case WTK_KEY_NUMPAD_1:
				case WTK_KEY_NUMPAD_2:
				case WTK_KEY_NUMPAD_3:
				case WTK_KEY_NUMPAD_4:
				case WTK_KEY_NUMPAD_5:
				case WTK_KEY_NUMPAD_6:
				case WTK_KEY_NUMPAD_7:
				case WTK_KEY_NUMPAD_8:
				case WTK_KEY_NUMPAD_9:
				case WTK_KEY_NUMPAD_PERIOD:
				case WTK_KEY_NUMPAD_ENTER:
				case WTK_KEY_NUMPAD_PLUS:
				case WTK_KEY_NUMPAD_MINUS:
				case WTK_KEY_NUMPAD_ASTERISK:
				case WTK_KEY_NUMPAD_SLASH:
					break;
				default: {
					int mkey_str = convert_key(key_sym_str);
					if(mkey_str != WTK_KEY_UNKOWN) {
						mkey = mkey_str;
					}
				} break;
			}

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			XIC xic = nullptr;
#endif

			utf8_buf = utf8_array;

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			if(xke->type == KeyPress) {
				xic = window->xic_;
				if(xic) {
					Status status;
					
					len = Xutf8LookupString(xic, xke, utf8_buf, sizeof(utf8_buf) - 5, &key_sym, &status);
					utf8_buf[len] = '\0';
					
					if(status == XBufferOverflow) {
						utf8_buf = (char *)MEM_mallocN(len + 5, "tiny_window::utf8_buf");
						len = Xutf8LookupString(xic, xke, utf8_buf, len, &key_sym, &status);
						utf8_buf[len] = '\0';
					}
				
					if (status == XLookupChars || status == XLookupBoth) {
						if ((utf8_buf[0] < 32 && utf8_buf[0] > 0) || (utf8_buf[0] == 127)) {
							utf8_buf[0] = '\0';
						}
					}
					else if(status == XLookupKeySym) {
						// This key doesn't have a text representation, it is a command key of some sort.
					}
					else {
						fprintf(stderr, "Bad keycode lookup. KeySym 0x%x Status: %s\n",
							static_cast<unsigned int>(key_sym),
							(status == XLookupNone ? "XLookupNone" : status == XLookupKeySym ? "XLookupKeySym" : "XUnkownStatus")
						);
					}
				}
				else {
					utf8_buf[0] = '\0';
				}
			}
#endif
			if (xke->type == KeyPress) {
				if (utf8_buf[0] == '\0' && ascii) {
					utf8_buf[0] = ascii;
					utf8_buf[1] = '\0';
				}
			}
			
			double time = static_cast<double>(evt->xkey.time) / 1000.0;
			
			switch(evt->type) {
				case KeyPress: {
					bool is_repeat_keycode = false;
					if(this->xkb_descr_ != nullptr) {
						if((xke->keycode < (XkbPerKeyBitArraySize << 3)) && bit_is_on(this->xkb_descr_->ctrls->per_key_repeat, xke->keycode)) {
							is_repeat_keycode = (this->keycode_last_repeat_key_ == xke->keycode);
						}
					}
					this->keycode_last_repeat_key_ = xke->keycode;
					
					if(KeyDownEvent) {
						KeyDownEvent(window, mkey, is_repeat_keycode, utf8_buf, time);
					}
				} break;
				case KeyRelease: {
					this->keycode_last_repeat_key_ = -1;
					
					if(KeyUpEvent) {
						KeyUpEvent(window, mkey, time);
					}
				} break;
			}
			
			if (utf8_buf != utf8_array) {
				MEM_freeN(utf8_buf);
			}
		} break;
		
		case ButtonPress: {
			double time = static_cast<double>(evt->xbutton.time) / 1000.0;
			
			switch(evt->xbutton.button) {
				case 1: {
					if(ButtonDownEvent) {
						ButtonDownEvent(window, WTK_BTN_LEFT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 2: {
					if(ButtonDownEvent) {
						ButtonDownEvent(window, WTK_BTN_MIDDLE, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 3: {
					if(ButtonDownEvent) {
						ButtonDownEvent(window, WTK_BTN_RIGHT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
			}
		} break;
		
		case ButtonRelease: {
			double time = static_cast<double>(evt->xbutton.time) / 1000.0;
			
			switch(evt->xbutton.button) {
				case 1: {
					if(ButtonUpEvent) {
						ButtonUpEvent(window, WTK_BTN_LEFT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 2: {
					if(ButtonUpEvent) {
						ButtonUpEvent(window, WTK_BTN_MIDDLE, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 3: {
					if(ButtonUpEvent) {
						ButtonUpEvent(window, WTK_BTN_RIGHT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 4: {
					if(WheelEvent) {
						WheelEvent(window, 0, 1, time);
					}
				} break;
				case 5: {
					if(WheelEvent) {
						WheelEvent(window, 0, -1, time);
					}
				} break;
			}
		} break;
		
		case FocusIn: {
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			if(window->xic_) {
				XSetICFocus(window->xic_);
			}
#endif
		} break;
		
		case FocusOut: {
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			if(window->xic_) {
				XUnsetICFocus(window->xic_);
			}
#endif
		} break;
		
		case MotionNotify: {
			double time = static_cast<double>(evt->xmotion.time) / 1000.0;
			
			if(MouseEvent) {
				MouseEvent(window, evt->xmotion.x, evt->xmotion.y, time);
			}
		} break;
		
		case GravityNotify: {
			// this is only supposed to pop up when the parent of this window(if any) has something happen
			// to it so that this window can react to said event as well.
		} break;
		
		// check for events that were created by the TinyWindow manager
		case ClientMessage: {
			if ((Atom)evt->xclient.data.l[0] == window->AtomClose) {
				window->should_close_ = true;
				if (DestroyEvent) {
					DestroyEvent(window);
				}
				break;
			}

			// check if full screen
			if ((Atom)evt->xclient.data.l[1] == window->AtomFullScreen) {
				break;
			}
			
			const char *atomName = XGetAtomName(this->display_, evt->xclient.message_type);
			if (atomName != nullptr) {
				// ?
			}
		} break;
	}
}
