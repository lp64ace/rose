#include "MEM_guardedalloc.h"

#include "x11_display.hh"
#include "x11_platform.hh"
#include "x11_window.hh"

#include <stdio.h>

namespace ghost {

bool X11Platform::init() {
	if(!(this->display_ = XOpenDisplay(nullptr))) {
		return false;
	}
	
	this->display_mngr_ = MEM_new<X11DisplayManager>("X11DisplayManager", this->display_);
	if(!this->display_mngr_->init()) {
		return false;
	}
	
	this->window_mngr_ = MEM_new<X11WindowManager>("X11WindowManager", this->display_);
	if(!this->window_mngr_->init()) {
		return false;
	}
	
	return true;
}

bool X11Platform::poll() {
	XEvent e;
	while(XPending(this->display_)) {
		XNextEvent(this->display_, &e);
		
		this->MsgProc(&e);
	}
	
	return true;
}

X11Platform::~X11Platform() {
	MEM_delete<X11WindowManager>(reinterpret_cast<X11WindowManager *>(this->window_mngr_));
	MEM_delete<X11DisplayManager>(reinterpret_cast<X11DisplayManager *>(this->display_mngr_));
	
	XCloseDisplay(this->display_);
}

void X11Platform::ClientMsgProc(XEvent *e, X11Window *window) {
	X11WindowManager *window_mngr = reinterpret_cast<X11WindowManager *>(this->window_mngr_);
	X11DisplayManager *display_mngr = reinterpret_cast<X11DisplayManager *>(this->display_mngr_);
	
	if ((Atom)e->xclient.data.l[0] == window_mngr->WM_DELETE_WINDOW) {
		this->window_mngr_->Close(window);
	}
}

void X11Platform::MsgProc(XEvent *e) {
	X11WindowManager *window_mngr = reinterpret_cast<X11WindowManager *>(this->window_mngr_);
	X11DisplayManager *display_mngr = reinterpret_cast<X11DisplayManager *>(this->display_mngr_);
	
	X11Window *window = window_mngr->Find(e->xany.window);
	if(!window) {
		return;
	}
	
	switch(e->type) {
		case ClientMessage: {
			this->ClientMsgProc(e, window);
		} break;
	}
}

} // namespace ghost
