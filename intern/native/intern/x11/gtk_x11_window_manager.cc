#include "gtk_x11_window_manager.hh"

#include <limits.h>
#include <stdio.h>

static int convert_key(const KeySym x11_key);
static int convert_code(XkbDescPtr xkb_descr, const KeyCode x11_code);
static int convert_key_ex(const KeySym x11_key, XkbDescPtr xkb_descr, const KeyCode x11_code);

GTKManagerX11::GTKManagerX11() : display(XOpenDisplay(NULL)) {
    if (!display) {
        return;
    }

    setlocale(LC_ALL, "");

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
    if (!this->InitXIM()){
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

bool GTKManagerX11::InitXIM() {
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

GTKWindowX11 *GTKManagerX11::GetWindowByHandle(Window handle) {
	for (auto window : this->GetWindows()) {
		if ((void *)static_cast<GTKWindowX11 *>(window)->GetHandle() == (void *)handle) {
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
			if (!this->xim) {
				this->InitXIM();
			}
			if (this->xim) {
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

static unsigned char bit_is_on(const unsigned char *ptr, int bit) {
	return ptr[bit >> 3] & (1 << (bit & 7));
}

void GTKManagerX11::EventProcedure(XEvent *evt) {
	GTKWindowX11 *window = this->GetWindowByHandle(evt->xany.window);
	
	if (!window) {
		return;
	}
	
	switch (evt->type) {
		case Expose: {
		} break;
		
		case DestroyNotify: {
			if (DispatchDestroy != nullptr) {
				DispatchDestroy(window);
			}
		} break;
		
		// when a request to resize the window is made either by dragging out the window or programmatically
		case ResizeRequest: {
			window->sizex = evt->xresizerequest.width;
			window->sizey = evt->xresizerequest.height;
			
			if (DispatchResize) {
				DispatchResize(window, window->sizex, window->sizey);
			}
		} break;

		// when a request to configure the window is made
		case ConfigureNotify: {
			// check if window was resized
			if (evt->xconfigure.width != window->sizex || evt->xconfigure.height != window->sizey) {
				window->sizex = evt->xconfigure.width;
				window->sizey = evt->xconfigure.height;
				
				if (DispatchResize) {
					DispatchResize(window, window->sizex, window->sizey);
				}
			}
			
			// check if window was moved
			if (evt->xconfigure.x != window->x || evt->xconfigure.y != window->y) {
				window->x = evt->xconfigure.x;
				window->y = evt->xconfigure.y;
				
				if (DispatchMove) {
					DispatchMove(window, window->x, window->y);
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

			XGetWindowProperty(this->display, evt->xproperty.window, window->AtomState, 0, LONG_MAX, false, AnyPropertyType, &type, &format, &numItems, &bytesAfter, &properties);

			if (properties && (format == 32)) {
				window->state = GTK_STATE_RESTORED;
				// go through each property and match it to an existing Atomic state
				for (unsigned int itemIndex = 0; itemIndex < numItems; itemIndex++) {
					Atom currentProperty = ((long *)(properties))[itemIndex];

					if (currentProperty == window->AtomHidden) {
						window->state = GTK_STATE_MINIMIZED;
					}

					if (currentProperty == window->AtomMaxVert || currentProperty == window->AtomMaxVert) {
						window->state = GTK_STATE_MAXIMIZED;
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
			if(!window->active) {
				break;
			}
			
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
			if ((xke->keycode >= 10 && xke->keycode < 20) && ((xke->state & number_hack_forbidden_kmods_mask) == 0)) {
				key_sym = XLookupKeysym(xke, ShiftMask);
				if (!(XK_0 <= key_sym && key_sym <= XK_9)) {
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
	
			int mkey = convert_key_ex(key_sym, this->xkb_descr, xke->keycode);
			switch (mkey) {
				case GTK_KEY_LEFT_ALT:
				case GTK_KEY_RIGHT_ALT:
				case GTK_KEY_LEFT_CTRL:
				case GTK_KEY_RIGHT_CTRL:
				case GTK_KEY_LEFT_SHIFT:
				case GTK_KEY_RIGHT_SHIFT:
				case GTK_KEY_LEFT_OS:
				case GTK_KEY_RIGHT_OS:
				case GTK_KEY_0:
				case GTK_KEY_1:
				case GTK_KEY_2:
				case GTK_KEY_3:
				case GTK_KEY_4:
				case GTK_KEY_5:
				case GTK_KEY_6:
				case GTK_KEY_7:
				case GTK_KEY_8:
				case GTK_KEY_9:
				case GTK_KEY_NUMPAD_0:
				case GTK_KEY_NUMPAD_1:
				case GTK_KEY_NUMPAD_2:
				case GTK_KEY_NUMPAD_3:
				case GTK_KEY_NUMPAD_4:
				case GTK_KEY_NUMPAD_5:
				case GTK_KEY_NUMPAD_6:
				case GTK_KEY_NUMPAD_7:
				case GTK_KEY_NUMPAD_8:
				case GTK_KEY_NUMPAD_9:
				case GTK_KEY_NUMPAD_PERIOD:
				case GTK_KEY_NUMPAD_ENTER:
				case GTK_KEY_NUMPAD_PLUS:
				case GTK_KEY_NUMPAD_MINUS:
				case GTK_KEY_NUMPAD_ASTERISK:
				case GTK_KEY_NUMPAD_SLASH:
					break;
				default: {
					int mkey_str = convert_key(key_sym_str);
					if (mkey_str != GTK_KEY_UNKOWN) {
						mkey = mkey_str;
					}
				} break;
			}

#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			XIC xic = nullptr;
#endif

			utf8_buf = utf8_array;
			
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			if (xke->type == KeyPress) {
				xic = window->xic;
				if (xic) {
					Status status;
					
					len = Xutf8LookupString(xic, xke, utf8_buf, sizeof(utf8_buf) - 5, &key_sym, &status);
					if (len >= 0) {
						utf8_buf[len] = '\0';
					}
					else {
						utf8_buf[0] = '\0';
					}
					
					if (status == XBufferOverflow) {
						utf8_buf = (char *)MEM_mallocN(len + 5, "Native::UTF8");
						len = Xutf8LookupString(xic, xke, utf8_buf, len, &key_sym, &status);
						if (len >= 0) {
							utf8_buf[len] = '\0';
						}
						else {
							utf8_buf[0] = '\0';
						}
					}
				
					if (status == XLookupChars || status == XLookupBoth) {
						if ((utf8_buf[0] < 32 && utf8_buf[0] > 0) || (utf8_buf[0] == 127)) {
							utf8_buf[0] = '\0';
						}
					}
					else if (status == XLookupKeySym) {
						// The keycode does not have a UTF8 representation.
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
#else
			if (xke->type == KeyPress) {
				if (utf8_buf[0] == '\0' && ascii) {
					utf8_buf[0] = ascii;
					utf8_buf[1] = '\0';
				}
			}
#endif
			
			double time = static_cast<double>(evt->xkey.time) / 1000.0;
			
			switch(evt->type) {
				case KeyPress: {
					bool is_repeat_keycode = false;
					if (this->xkb_descr != nullptr) {
						if ((xke->keycode < (XkbPerKeyBitArraySize << 3)) && bit_is_on(this->xkb_descr->ctrls->per_key_repeat, xke->keycode)) {
							is_repeat_keycode = (this->keycode_last_repeat_key == xke->keycode);
						}
					}
					this->keycode_last_repeat_key = xke->keycode;

					if (DispatchKeyDown) {
						DispatchKeyDown(window, mkey, is_repeat_keycode, utf8_buf, time);
					}

					if (xic) {
						int i = 0;
						while (true) {
							if (((unsigned char *)utf8_buf)[i++] > 0x7f) {
								for (; i < len; i++) {
									if (0xbf < ((unsigned char *)utf8_buf)[i] && ((unsigned char *)utf8_buf)[i] < 0x80) {
										break;
									}
								}
							}

							if (i >= len) {
								break;
							}

							if (DispatchKeyDown) {
								DispatchKeyDown(window, mkey, is_repeat_keycode, &utf8_buf[i], time);
							}
						}
					}
				} break;
				case KeyRelease: {
					this->keycode_last_repeat_key = -1;
					
					if (DispatchKeyUp) {
						DispatchKeyUp(window, mkey, time);
					}
				} break;
			}
			
			if (utf8_buf != utf8_array) {
				MEM_freeN(utf8_buf);
			}
		} break;
		
		case ButtonPress: {
			if(!window->active) {
				break;
			}
			
			double time = static_cast<double>(evt->xbutton.time) / 1000.0;
			
			switch(evt->xbutton.button) {
				case 1: {
					if (DispatchButtonDown) {
						DispatchButtonDown(window, GTK_BTN_LEFT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 2: {
					if (DispatchButtonDown) {
						DispatchButtonDown(window, GTK_BTN_MIDDLE, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 3: {
					if (DispatchButtonDown) {
						DispatchButtonDown(window, GTK_BTN_RIGHT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
			}
		} break;
		
		case ButtonRelease: {
			if(!window->active) {
				break;
			}
			
			double time = static_cast<double>(evt->xbutton.time) / 1000.0;
			
			switch(evt->xbutton.button) {
				case 1: {
					if (DispatchButtonUp) {
						DispatchButtonUp(window, GTK_BTN_LEFT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 2: {
					if (DispatchButtonUp) {
						DispatchButtonUp(window, GTK_BTN_MIDDLE, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 3: {
					if (DispatchButtonUp) {
						DispatchButtonUp(window, GTK_BTN_RIGHT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 4: {
					if (DispatchWheel) {
						DispatchWheel(window, 0, 1, time);
					}
				} break;
				case 5: {
					if (DispatchWheel) {
						DispatchWheel(window, 0, -1, time);
					}
				} break;
			}
		} break;
		
		case FocusIn: {
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			if (window->xic) {
				XSetICFocus(window->xic);
			}
#endif
			if(DispatchFocus) {
				DispatchFocus(window, true);
			}
			window->active = true;
		} break;
		
		case FocusOut: {
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			if (window->xic) {
				XUnsetICFocus(window->xic);
			}
#endif
			if(DispatchFocus) {
				DispatchFocus(window, false);
			}
			window->active = false;
		} break;
		
		case MotionNotify: {
			if(!window->active) {
				break;
			}
			
			double time = static_cast<double>(evt->xmotion.time) / 1000.0;
			
			if (DispatchMouse) {
				DispatchMouse(window, evt->xmotion.x, evt->xmotion.y, time);
			}
		} break;
		
		case GravityNotify: {
			// this is only supposed to pop up when the parent of this window(if any) has something happen
			// to it so that this window can react to said event as well.
		} break;
		
		// check for events that were created by the TinyWindow manager
		case ClientMessage: {
			if ((Atom)evt->xclient.data.l[0] == window->AtomClose) {
				if (DispatchDestroy) {
					DispatchDestroy(window);
				}
				break;
			}

			// check if full screen
			if ((Atom)evt->xclient.data.l[1] == window->AtomFullScreen) {
				break;
			}
			
			const char *atomName = XGetAtomName(this->display, evt->xclient.message_type);
			if (atomName != nullptr) {
				// ?
			}
		} break;
	}
}

static int convert_key(const KeySym x11_key) {
	int key = GTK_KEY_UNKOWN;
	
	if ((x11_key >= XK_A) && (x11_key <= XK_Z)) {
		key = (x11_key - XK_A + GTK_KEY_A);
	}
	else if ((x11_key >= XK_a) && (x11_key <= XK_z)) {
		key = (x11_key - XK_a + GTK_KEY_A);
	}
	else if ((x11_key >= XK_0) && (x11_key <= XK_9)) {
		key = (x11_key - XK_0 + GTK_KEY_0);
	}
	else if ((x11_key >= XK_F1) && (x11_key <= XK_F24)) {
		key = (x11_key - XK_F1 + GTK_KEY_F1);
	}
	else {
		
#define ASSOCIATE(store, what, to) case what: store = to; break;
		
		switch(x11_key) {
			ASSOCIATE(key, XK_BackSpace, GTK_KEY_BACKSPACE);
			ASSOCIATE(key, XK_Tab, GTK_KEY_TAB);
			ASSOCIATE(key, XK_ISO_Left_Tab, GTK_KEY_TAB);
			ASSOCIATE(key, XK_Return, GTK_KEY_ENTER);
			ASSOCIATE(key, XK_Escape, GTK_KEY_ESC);
			ASSOCIATE(key, XK_space, GTK_KEY_SPACE);
			
			ASSOCIATE(key, XK_semicolon, GTK_KEY_SEMICOLON);
			ASSOCIATE(key, XK_period, GTK_KEY_PERIOD);
			ASSOCIATE(key, XK_comma, GTK_KEY_COMMA);
			ASSOCIATE(key, XK_quoteright, GTK_KEY_QUOTE);
			ASSOCIATE(key, XK_quoteleft, GTK_KEY_ACCENTGRAVE);
			ASSOCIATE(key, XK_minus, GTK_KEY_MINUS);
			ASSOCIATE(key, XK_plus, GTK_KEY_PLUS);
			ASSOCIATE(key, XK_slash, GTK_KEY_SLASH);
			ASSOCIATE(key, XK_backslash, GTK_KEY_BACKSLASH);
			ASSOCIATE(key, XK_equal, GTK_KEY_EQUAL);
			ASSOCIATE(key, XK_bracketleft, GTK_KEY_LEFT_BRACKET);
			ASSOCIATE(key, XK_bracketright, GTK_KEY_RIGHT_BRACKET);
			ASSOCIATE(key, XK_Pause, GTK_KEY_PAUSE);
			
			ASSOCIATE(key, XK_Shift_L, GTK_KEY_LEFT_SHIFT);
			ASSOCIATE(key, XK_Shift_R, GTK_KEY_RIGHT_SHIFT);
			ASSOCIATE(key, XK_Control_L, GTK_KEY_LEFT_CTRL);
			ASSOCIATE(key, XK_Control_R, GTK_KEY_RIGHT_CTRL);
			ASSOCIATE(key, XK_Alt_L, GTK_KEY_LEFT_ALT);
			ASSOCIATE(key, XK_Alt_R, GTK_KEY_RIGHT_ALT);
			ASSOCIATE(key, XK_Super_L, GTK_KEY_LEFT_OS);
			ASSOCIATE(key, XK_Super_R, GTK_KEY_RIGHT_OS);
			
			ASSOCIATE(key, XK_Insert, GTK_KEY_INSERT);
			ASSOCIATE(key, XK_Delete, GTK_KEY_DELETE);
			ASSOCIATE(key, XK_Home, GTK_KEY_HOME);
			ASSOCIATE(key, XK_End, GTK_KEY_END);
			ASSOCIATE(key, XK_Page_Up, GTK_KEY_PAGEUP);
			ASSOCIATE(key, XK_Page_Down, GTK_KEY_PAGEDOWN);
			
			ASSOCIATE(key, XK_Left, GTK_KEY_LEFT);
			ASSOCIATE(key, XK_Right, GTK_KEY_RIGHT);
			ASSOCIATE(key, XK_Up, GTK_KEY_UP);
			ASSOCIATE(key, XK_Down, GTK_KEY_DOWN);
			
			ASSOCIATE(key, XK_Caps_Lock, GTK_KEY_CAPSLOCK);
			ASSOCIATE(key, XK_Scroll_Lock, GTK_KEY_SCRLOCK);
			ASSOCIATE(key, XK_Num_Lock, GTK_KEY_NUMLOCK);
			ASSOCIATE(key, XK_Menu, GTK_KEY_APP);
			
			ASSOCIATE(key, XK_KP_0, GTK_KEY_NUMPAD_0);
			ASSOCIATE(key, XK_KP_1, GTK_KEY_NUMPAD_1);
			ASSOCIATE(key, XK_KP_2, GTK_KEY_NUMPAD_2);
			ASSOCIATE(key, XK_KP_3, GTK_KEY_NUMPAD_3);
			ASSOCIATE(key, XK_KP_4, GTK_KEY_NUMPAD_4);
			ASSOCIATE(key, XK_KP_5, GTK_KEY_NUMPAD_5);
			ASSOCIATE(key, XK_KP_6, GTK_KEY_NUMPAD_6);
			ASSOCIATE(key, XK_KP_7, GTK_KEY_NUMPAD_7);
			ASSOCIATE(key, XK_KP_8, GTK_KEY_NUMPAD_8);
			ASSOCIATE(key, XK_KP_9, GTK_KEY_NUMPAD_9);
			ASSOCIATE(key, XK_KP_Decimal, GTK_KEY_NUMPAD_PERIOD);
			
			ASSOCIATE(key, XK_KP_Insert, GTK_KEY_NUMPAD_0);
			ASSOCIATE(key, XK_KP_End, GTK_KEY_NUMPAD_1);
			ASSOCIATE(key, XK_KP_Down, GTK_KEY_NUMPAD_2);
			ASSOCIATE(key, XK_KP_Page_Down, GTK_KEY_NUMPAD_3);
			ASSOCIATE(key, XK_KP_Left, GTK_KEY_NUMPAD_4);
			ASSOCIATE(key, XK_KP_Begin, GTK_KEY_NUMPAD_5);
			ASSOCIATE(key, XK_KP_Right, GTK_KEY_NUMPAD_6);
			ASSOCIATE(key, XK_KP_Home, GTK_KEY_NUMPAD_7);
			ASSOCIATE(key, XK_KP_Up, GTK_KEY_NUMPAD_8);
			ASSOCIATE(key, XK_KP_Page_Up, GTK_KEY_NUMPAD_9);
			ASSOCIATE(key, XK_KP_Delete, GTK_KEY_NUMPAD_PERIOD);
			
			ASSOCIATE(key, XK_KP_Enter, GTK_KEY_NUMPAD_ENTER);
			ASSOCIATE(key, XK_KP_Add, GTK_KEY_NUMPAD_PLUS);
			ASSOCIATE(key, XK_KP_Subtract, GTK_KEY_NUMPAD_MINUS);
			ASSOCIATE(key, XK_KP_Multiply, GTK_KEY_NUMPAD_ASTERISK);
			ASSOCIATE(key, XK_KP_Divide, GTK_KEY_NUMPAD_SLASH);

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
	}
	return GTK_KEY_UNKOWN;
}

static int convert_key_ex(const KeySym x11_key, XkbDescPtr xkb_descr, const KeyCode x11_code) {
	int key = convert_key(x11_key);
	if (key == GTK_KEY_UNKOWN) {
		if (xkb_descr) {
			key = convert_code(xkb_descr, x11_code);
		}
	}
	return key;
}
