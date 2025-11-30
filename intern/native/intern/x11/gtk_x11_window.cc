#include "MEM_guardedalloc.h"

#include "gtk_x11_window.hh"

GTKWindowX11::GTKWindowX11(GTKManagerX11 *manager) : GTKWindowInterface(manager), window(NULL), visual_info(NULL) {
	this->display = manager->display;
}

GTKWindowX11::~GTKWindowX11() {
	if (this->window) {
		XDestroyWindow(this->display, this->window);
		this->window = NULL;
	}
}

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
XIC xic;

bool GTKWindowX11::InitXIC(GTKManagerX11 *manager) {
	XIM xim = manager->xim;
	if (!xim) {
		return false;
	}
	XICCallback destroy;
	destroy.callback = (XICProc)destroyICCallback;
	destroy.client_data = (XPointer)&this->xic;
	this->xic = XCreateIC(
		xim,
		XNClientWindow, this->window_handle_,
		XNFocusWindow, this->window_handle_,
		XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
		XNDestroyCallback, &destroy,
		nullptr
	);
	
	if (!this->xic) {
		return false;
	}
	
	unsigned long mask = 0;
		
	mask |= KeyPressMask | KeyReleaseMask;
	mask |= ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
	mask |= PointerMotionMask | EnterWindowMask | LeaveWindowMask;
	mask |= SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask | FocusChangeMask;
	mask |= ExposureMask;
	
	unsigned long extra = 0;
	XGetICValues(this->xic, XNFilterEvents, &extra, nullptr);
	XSelectInput(this->display, this->window, mask | extra);
	return true;
}
#endif

bool GTKWindowX11::Create(GTKWindowInterface *parent, const char *name, int width, int height, int state) {
	int glattribs[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, settings.color_bits,
		GLX_GREEN_SIZE, settings.color_bits,
		GLX_BLUE_SIZE, settings.color_bits,
		GLX_ALPHA_SIZE, settings.color_bits,
		GLX_DEPTH_SIZE, settings.depth_bits,
		GLX_STENCIL_SIZE, settings.stencil_bits,
		None
	};
	
	this->visual_info = glXChooseVisual(this->display, XDefaultScreen(this->display), glattribs);
	
	if (!this->visual_info) {
		return false;
	}
	
	XSetWindowAttributes xattribs;
	xattribs.colormap = XCreateColormap(this->display_, DefaultRootWindow(this->display_), this->visual_info_->visual, AllocNone);
	xattribs.event_mask |= ExposureMask;
	xattribs.event_mask |= KeyPressMask | KeyReleaseMask;
	xattribs.event_mask |= PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	xattribs.event_mask |= FocusIn | FocusOut | FocusChangeMask;
	xattribs.event_mask |= Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask;
	xattribs.event_mask |= VisibilityChangeMask | PropertyChangeMask | SubstructureNotifyMask | MotionNotify;
	
	this->window = XCreateWindow(this->display, XDefaultRootWindow(this->display), 0, 0, width, height, 0, this->visual_info->depth, InputOutput, this->visual_info->visual, CWColormap | CWEventMask, &xattribs);

	if (!this->window) {
		return false;
	}
	
	this->AtomState = XInternAtom(this->display, "_NET_WM_STATE", false);
	this->AtomFullScreen = XInternAtom(this->display, "_NET_WM_STATE_FULLSCREEN", false);
	this->AtomMaxHorz = XInternAtom(this->display, "_NET_WM_STATE_MAXIMIZED_HORZ", false);
	this->AtomMaxVert = XInternAtom(this->display, "_NET_WM_STATE_MAXIMIZED_VERT", false);
	this->AtomClose = XInternAtom(this->display, "WM_DELETE_WINDOW", false);
	this->AtomHidden = XInternAtom(this->display, "_NET_WM_STATE_HIDDEN", false);
	this->AtomActive = XInternAtom(this->display, "_NET_ACTIVE_WINDOW", false);
	this->AtomDemandsAttention = XInternAtom(this->display, "_NET_WM_STATE_DEMANDS_ATTENTION", false);
	this->AtomFocused = XInternAtom(this->display, "_NET_WM_STATE_FOCUSED", false);
	this->AtomCardinal = XInternAtom(this->display, "CARDINAL", false);
	this->AtomIcon = XInternAtom(this->display, "_NET_WM_ICON", false);
	this->AtomHints = XInternAtom(this->display, "_MOTIF_WM_HINTS", true);

	this->AtomWindowType = XInternAtom(this->display, "_NET_WM_WINDOW_TYPE", false);
	this->AtomWindowTypeDesktop = XInternAtom(this->display, "_NET_WM_WINDOW_TYPE_UTILITY", false);
	this->AtomWindowTypeSplash = XInternAtom(this->display, "_NET_WM_WINDOW_TYPE_SPLASH", false);
	this->AtomWindowTypeNormal = XInternAtom(this->display, "_NET_WM_WINDOW_TYPE_NORMAL", false);

	this->AtomAllowedActions = XInternAtom(this->display, "_NET_WM_ALLOWED_ACTIONS", false);
	this->AtomActionResize = XInternAtom(this->display, "WM_ACTION_RESIZE", false);
	this->AtomActionMinimize = XInternAtom(this->display, "_WM_ACTION_MINIMIZE", false);
	this->AtomActionShade = XInternAtom(this->display, "WM_ACTION_SHADE", false);
	this->AtomActionMaximizeHorz = XInternAtom(this->display, "_WM_ACTION_MAXIMIZE_HORZ", false);
	this->AtomActionMaximizeVert = XInternAtom(this->display, "_WM_ACTION_MAXIMIZE_VERT", false);
	this->AtomActionClose = XInternAtom(this->display, "_WM_ACTION_CLOSE", false);

	this->AtomDesktopGeometry = XInternAtom(this->display, "_NET_DESKTOP_GEOMETRY", false);
	
	XStoreName(this->display, this->window, name);
	
	unsigned long mask = 0;
		
	mask |= KeyPressMask | KeyReleaseMask;
	mask |= ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
	mask |= PointerMotionMask | EnterWindowMask | LeaveWindowMask;
	mask |= SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask | FocusChangeMask;
	mask |= ExposureMask;
	
	XSelectInput(this->display, this->window, mask);
	XSetWMProtocols(this->display, this->window, &this->AtomClose, 1);
	
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
	if (!this->InitXIC(static_cast<GTKManagerX11 *>(this->GetManagerInterface()))) {
		return false;
	}
#endif

	return true;
}

void GTKWindowX11::Minimize() {
}
void GTKWindowX11::Maximize() {
}
void GTKWindowX11::Show() {
	XMapWindow(this->display, this->window);
}
void GTKWindowX11::Hide() {
	XUnmapWindow(this->display, this->window);
}

int GTKWindowX11::GetState(void) const {
	return this->state;
}

void GTKWindowX11::GetPos(int *x, int *y) const {
	XWindowAttributes attr;
	
	XGetWindowAttributes(this->display, this->window, &attr);
	
	if (x) {
		*x = attr.x;
	}
	if (y) {
		*y = attr.y;
	}
}

void GTKWindowX11::GetSize(int *w, int *h) const {
	XWindowAttributes attr;
	
	XGetWindowAttributes(this->display, this->window, &attr);
	
	if (w) {
		*w = attr.width;
	}
	if (h) {
		*h = attr.height;
	}
}

Window GTKWindowX11::GetHandle() {
	return this->window;
}

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
	else if ((x11_key >= XK_F1) && (x11_key <= XK_F24)) {
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

			default: {
				// fprintf(stdout, "[Linux/Key] Warning! Failed to associate %lu with any key.\n", x11_key);
			} break;
		}
		
#undef ASSOCIATE
	}
	
	return key;
}

static int convert_code(XkbDescPtr xkb_descr, const KeyCode x11_code) {
	if (x11_code >= xkb_descr->min_key_code && x11_code <= xkb_descr->max_key_code) {
		const char *id_str = xkb_descr->names->keys[x11_code].name;
		fprintf(stdout, "[Linux/Key] %s\n", id_str);
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

static unsigned char bit_is_on(const unsigned char *ptr, int bit) {
	return ptr[bit >> 3] & (1 << (bit & 7));
}
