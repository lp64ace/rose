#include "MEM_guardedalloc.h"

#include "gtk_x11_window.hh"
#include "gtk_x11_window_manager.hh"

GTKWindowX11::GTKWindowX11(GTKManagerX11 *manager) : GTKWindowInterface(manager), window(), visual_info(NULL) {
	this->display = manager->display;
}

GTKWindowX11::~GTKWindowX11() {
	if (this->window) {
		XDestroyWindow(this->display, this->window);
	}
}

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
static Bool DestroyICCallback(XIC xic, XPointer ptr, XPointer data) {
	if (ptr) {
		*(XIC *)ptr = nullptr;
	}
	/* Ignored by X11. */
	return True;
}

bool GTKWindowX11::InitXIC(GTKManagerX11 *manager) {
	XIM xim = manager->xim;
	if (!xim) {
		return false;
	}
	XICCallback destroy;
	destroy.callback = (XICProc)DestroyICCallback;
	destroy.client_data = (XPointer)&this->xic;
	this->xic = XCreateIC(
		xim,
		XNClientWindow, this->window,
		XNFocusWindow, this->window,
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
	XSetWindowAttributes xattribs;
	xattribs.event_mask |= ExposureMask;
	xattribs.event_mask |= KeyPressMask | KeyReleaseMask;
	xattribs.event_mask |= PointerMotionMask | ButtonPressMask | ButtonReleaseMask;
	xattribs.event_mask |= FocusIn | FocusOut | FocusChangeMask;
	xattribs.event_mask |= Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask;
	xattribs.event_mask |= VisibilityChangeMask | PropertyChangeMask | SubstructureNotifyMask | MotionNotify;
	
	this->window = XCreateWindow(this->display, XDefaultRootWindow(this->display), 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &xattribs);

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

GTKRenderInterface *GTKWindowX11::AllocateRender(int render) {
	switch (render) {
		case GTK_WINDOW_RENDER_OPENGL: {
			return static_cast<GTKRenderInterface *>(MEM_new<GTKRenderXGL>("GTKRenderXGL", this));
		} break;
	}
	return NULL;
}
