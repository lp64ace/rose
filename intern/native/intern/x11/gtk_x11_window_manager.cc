#include "gtk_x11_window_manager.hh"

GTKManagerX11::GTKManagerX11() : display(XOpenDisplay(NULL)) {
    if (!display) {
        return;
    }

    setlocale(LC_ALL, "");

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
    if (!this->XImmediateInit()){
        // Not really a reason to abort since some systems do not have it completely.
    }
#endif

    /* Init Keyboard Description */

    int use_xkb;
    int xkb_opc, xkb_evt, xkb_err;
    int xkb_mjr = XkbMajorVersion, xkb_mnr = XkbMinorVersion;

    if ((use_xkb = XkbQueryExtension(this->display, &xkb_opc, &xkb_evt, &xkb_err, &xkb_mjr, &xkb_mnr))) {
        XkbSetDetectableAutoRepeat(this->display, true, NULL);

        this->xkb_descr = XkbGetMap(this->display, 0, XkbUseCoreKbd);
        if (!this->xkb_descr) {
            return;
        }

        XkbGetNames(this->display, XkbKeyNamesMask, this->xkb_descr);
        XkbGetControls(this->display, XkbPerKeyRepeatMask | XkbRepeatKeysMask, this->xkb_descr);
    }
	
	this->XExtensionsInit();
}

GTKManagerX11::~GTKManagerX11() {
    if (this->xkb_descr) {
        XkbFreeKeyboard(this->xkb_descr, XkbAllComponentsMask, true);
    }
    if (this->display) {
        XCloseDisplay(this->display);
    }
}

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
static void XImmediateDestroyCallback(XIM xim, XPointer ptr, XPointer data) {
    if (ptr) {
        *(XIM *)ptr = NULL;
    }
}

bool GTKManagerX11::XImmediateInit() {
    XSetLocaleModifiers("");

    if (!(this->xim = XOpenIM(this->display, NULL, NULL, NULL))) {
        return false;
    }

    XIMCallback destroy;
    destroy.callback = (XIMProc)XImmediateDestroyCallback;
    destroy.client_data = (XPointer)&this->xim;
    XSetIMValues(this->xim, XNDestroyCallback, &destroy, NULL);

    return true;
}
#endif

bool GTKManagerX11::XExtensionsInit() {
	if (!(glxCreateContextAttribsARB = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const unsigned char*)"glXCreateContextAttribsARB"))) {
		return false;
	}
	if (!(glxMakeContextCurrent = (PFNGLXMAKECONTEXTCURRENTPROC)glXGetProcAddress((const unsigned char*)"glXMakeContextCurrent"))) {
		return false;
	}
	if (!(glxSwapIntervalEXT = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const unsigned char*)"glXSwapIntervalEXT"))) {
		// return false;
	}
	if (!(glxChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)glXGetProcAddress((const unsigned char*)"glXChooseFBConfig"))) {
		return false;
	}
	return true;
}

GTKWindowX11 *GTKManagerX11::GetWindowByHandle(Window window) {
	for (auto window : this->GetWindows()) {
		if (static_cast<GTKWindowX11 *>(window)->GetHandle() == window) {
			return static_cast<GTKWindowX11 *>(window);
		}
	}
	return NULL;
}

bool GTKManagerX11::HasEvents() {
	if (XEventsQueued(this->display, QueuedAfterReading)) {
		return true;
	}
	return false;
}

void GTKManagerX11::Poll() {
	XEvent evt;
	while (XPending(this->display)) {
		XNextEvent(this->display, &evt);
		
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
		if (evt.type == FocusIn || evt.type == KeyPress) {
			if (!this->xim_) {
				this->InitXIM();
			}
			if (this->xim_) {
				GTKWindowX11 *window = this->GetWindowByHandle(evt.xany.window);
				if (window && !window->xic && window->InitXIC(this)) {
					if (evt.type == KeyPress) {
						XSetICFocus(window->xic);
					}
				}
			}
		}

		if (XFilterEvent(&evt, (Window)NULL) == True) {
			// Do nothing now, the event is consumed by XIM.
			continue;
		}
#endif
		EventProcedure(&evt);
	}
}
