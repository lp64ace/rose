#include "MEM_guardedalloc.h"

#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "x11_window.hh"

#include "glib.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

namespace ghost {

X11Window::X11Window(X11WindowManager *manager) : manager_(manager), handle_((Window)0) {
}

bool X11Window::init(X11Window *parent, const char *title, int x, int y, int width, int height) {
	Display *display = this->manager_->display_;
	
	this->handle_ = XCreateSimpleWindow(
		display,
		(parent) ? parent->handle_ : DefaultRootWindow(display),
		(x == 0xffff) ? 0 : x,
		(y == 0xffff) ? 0 : y,
		width,
		height,
		0,
		0,
		0xffffffff
	);
	
	if(!this->handle_) {
		return false;
	}
	
	Atom protocols[] = {
		this->manager_->WM_DELETE_WINDOW,
	};
	XSetWMProtocols(display, this->handle_, protocols, sizeof(protocols)/sizeof(protocols[0]));
	XSelectInput(display, this->handle_, ExposureMask | KeyPressMask | ButtonPressMask | StructureNotifyMask | SubstructureNotifyMask);
	
	this->SetTitle(title);
	
	return true;
}

X11Window::~X11Window() {
	Display *display = this->manager_->display_;
	Window handle = this->handle_;
	
	if(!this->handle_) {
		return;
	}
	
	this->handle_ = (Window)0;
	
	XDestroyWindow(display, handle);
}

bool X11Window::exit() {
	if(this->manager_) {
		return this->manager_->Close(this);
	}
	return false;
}

void X11Window::SetTitle(const char *title) {
	Display *display = this->manager_->display_;
	Window handle = this->handle_;
	
	Atom name = XInternAtom(display, "_NET_WM_NAME", 0);
	Atom utf8str = XInternAtom(display, "UTF8_STRING", 0);
	XChangeProperty(display, handle, name, utf8str, 8, PropModeReplace, (const unsigned char *)title, strlen(title));
	XStoreName(display, handle, title);
	XFlush(display);
}

#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1

static void show_default(X11WindowManager *manager, Window window, bool set) {
	Display *display = manager->display_;
	
	XEvent e;
	memset(&e, 0, sizeof(XEvent));
	
	e.xclient.type = ClientMessage;
	e.xclient.serial = 0;
	e.xclient.send_event = True;
	e.xclient.window = window;
	e.xclient.message_type = manager->WM_CHANGE_STATE;
	e.xclient.format = 32;
	
	e.xclient.data.l[0] = NormalState;
	
	if(set) {
		XMapWindow(display, window);
	}
	else {
		XUnmapWindow(display, window);
	}
	
	XSendEvent(display, window, False, SubstructureNotifyMask | SubstructureRedirectMask, &e);
	XFlush(display);
}

static void show_maximize(X11WindowManager *manager, Window window, bool set) {
	Display *display = manager->display_;

	XEvent e;
	memset(&e, 0, sizeof(XEvent));
	
	e.xclient.type = ClientMessage;
	e.xclient.serial = 0;
	e.xclient.send_event = True;
	e.xclient.window = window;
	e.xclient.message_type = manager->_NET_WM_STATE;
	e.xclient.format = 32;
	
	if(set) {
		e.xclient.data.l[0] = _NET_WM_STATE_ADD;
	}
	else {
		e.xclient.data.l[0] = _NET_WM_STATE_REMOVE;
	}
	
	e.xclient.data.l[1] = manager->_NET_WM_STATE_MAXIMIZED_HORZ;
	e.xclient.data.l[2] = manager->_NET_WM_STATE_MAXIMIZED_VERT;
	e.xclient.data.l[3] = 0;
	e.xclient.data.l[4] = 0;
	
	XSendEvent(display, DefaultRootWindow(display), False, SubstructureRedirectMask | SubstructureNotifyMask, &e);
	XFlush(display);
}

static void show_minimize(X11WindowManager *manager, Window window, bool set) {
	Display *display = manager->display_;
	
	XEvent e;
	memset(&e, 0, sizeof(XEvent));
	
	e.xclient.type = ClientMessage;
	e.xclient.serial = 0;
	e.xclient.send_event = True;
	e.xclient.window = window;
	e.xclient.message_type = manager->WM_CHANGE_STATE;
	e.xclient.format = 32;
	
	if(set) {
		e.xclient.data.l[0] = IconicState;
	}
	else {
		e.xclient.data.l[0] = NormalState;
	}
	
	XSendEvent(display, DefaultRootWindow(display), False, SubstructureNotifyMask | SubstructureRedirectMask, &e);
	XFlush(display);
}

#undef _NET_WM_STATE_REMOVE
#undef _NET_WM_STATE_ADD

static bool is_maximized(X11WindowManager *manager, Window window) {
	Atom *prop = NULL, type;
	Display *display = manager->display_;
	
	unsigned long after, num;
	
	int format;
	if(XGetWindowProperty(display, window, manager->_NET_WM_STATE, 0, INT_MAX, False, XA_ATOM, &type, &format, &num, &after, (unsigned char **)&prop) != Success) {
		return false;
	}
	if(prop == NULL) {
		return false;
	}
	
	int dimensions = 0;
	if (type == XA_ATOM && format == 32) {
		for(ulong i = 0; i < num; i++) {
			if (prop[i] == manager->_NET_WM_STATE_MAXIMIZED_HORZ) {
				dimensions++;
			}
			if (prop[i] == manager->_NET_WM_STATE_MAXIMIZED_VERT) {
				dimensions++;
			}
		}
	}
	XFree(prop);
	
	return dimensions == 2;
}

static bool is_minimized(X11WindowManager *manager, Window window) {
	Atom *prop = NULL, type;
	Display *display = manager->display_;
	
	unsigned long after, num;
	
	int format;
	if(XGetWindowProperty(display, window, manager->_NET_WM_STATE, 0, INT_MAX, False, XA_ATOM, &type, &format, &num, &after, (unsigned char **)&prop) != Success) {
		return false;
	}
	if(prop == NULL || num == 0) {
		return false;
	}
	
	int hidden = 0;
	if (type == XA_ATOM && format == 32) {
		for(ulong i = 0; i < num; i++) {
			if (prop[i] == manager->_NET_WM_STATE_HIDDEN) {
				hidden++;
			}
		}
	}
	XFree(prop);
	
	return hidden > 0;
}

bool X11Window::Show(int command) {
	Display *display = this->manager_->display_;
	
	bool maximized = is_maximized(this->manager_, this->handle_);
	bool minimized = is_minimized(this->manager_, this->handle_);
	
	switch(command) {
		case WTK_SW_DEFAULT:
		case WTK_SW_SHOW: {
			if(maximized) {
				show_maximize(this->manager_, this->handle_, false);
			}
			show_default(this->manager_, this->handle_, true);
		} break;
		case WTK_SW_HIDE: {
			show_default(this->manager_, this->handle_, false);
		} break;
		case WTK_SW_MINIMIZE: {
			if(maximized) {
				show_maximize(this->manager_, this->handle_, false);
			}
			show_minimize(this->manager_, this->handle_, true);
		} break;
		case WTK_SW_MAXIMIZE: {
			show_maximize(this->manager_, this->handle_, true);
		} break;
	}
	
	return true;
}

bool X11Window::GetPos(int *r_left, int *r_top) const {
	Display *display = this->manager_->display_;
	
	Window parent;
	
	unsigned int w, h, border, depth;
	XGetGeometry(display, this->handle_, &parent, r_left, r_top, &w, &h, &border, &depth);
	return true;
}
bool X11Window::GetSize(int *r_width, int *r_height) const {
	Display *display = this->manager_->display_;
	
	Window parent;
	
	int x, y;
	unsigned int w, h, border, depth;
	XGetGeometry(display, this->handle_, &parent, &x, &y, &w, &h, &border, &depth);
	*r_width = (int)w;
	*r_height = (int)h;
	return true;
}

bool X11Window::SetPos(int left, int top) {
	Display *display = this->manager_->display_;
	
	XWindowChanges values;
	values.x = left;
	values.y = top;
	XConfigureWindow(display, this->handle_, CWX | CWY, &values);
	return true;
}
bool X11Window::SetSize(int width, int height) {
	Display *display = this->manager_->display_;
	
	XWindowChanges values;
	values.width = width;
	values.height = height;
	XConfigureWindow(display, this->handle_, CWWidth | CWHeight, &values);
	return true;
}

X11WindowManager::X11WindowManager(Display *display) : display_(display) {
}

bool X11WindowManager::init() {
	Display *display = this->display_;
	
#define GTK_INTERN_ATOM(atom)									  \
	{															  \
		atom = XInternAtom(display, #atom, False);				  \
		if(atom == None) {										  \
			fprintf(stderr, "Failed to initialize %s.\n", #atom); \
			return false;										  \
		}														  \
	}															  \
	(void)0
	
	GTK_INTERN_ATOM(WM_DELETE_WINDOW);
	GTK_INTERN_ATOM(WM_CHANGE_STATE);
	
	GTK_INTERN_ATOM(_NET_WM_STATE);
	GTK_INTERN_ATOM(_NET_WM_STATE_MAXIMIZED_HORZ);
	GTK_INTERN_ATOM(_NET_WM_STATE_MAXIMIZED_VERT);
	GTK_INTERN_ATOM(_NET_WM_STATE_HIDDEN);
	
#undef GTK_INTERN_ATOM
	
	return true;
}

WindowBase *X11WindowManager::Spawn(WindowBase *vparent, const char *title, int x, int y, int w, int h) {
	X11Window *parent = reinterpret_cast<X11Window *>(vparent);
	X11Window *window = MEM_new<X11Window>("X11Window", this);
	if (!window->init(parent, title, x, y, w, h)) {
		MEM_delete<X11Window>(window);
		return NULL;
	}
	WindowBase *vwindow = reinterpret_cast<WindowBase *>(window);
	this->windows_.push_back(vwindow);
	return vwindow;
}
X11Window *X11WindowManager::Find(Window handle) {
	for(std::vector<WindowBase *>::iterator itr = this->windows_.begin(); itr != this->windows_.end(); itr++) {
		X11Window *window = reinterpret_cast<X11Window *>(*itr);
		if(window->handle_ == handle) {
			return window;
		}
	}
	return NULL;
}

}
