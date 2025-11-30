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

void GTKManagerX11::EventProcedure(XEvent *evt) {
	GTKWindowX11 *window = this->GetWindowByHandle(evt->xany.window);
	
	if (!window) {
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

			XGetWindowProperty(this->display, evt->xproperty.window, window->AtomState, 0, LONG_MAX, false, AnyPropertyType, &type, &format, &numItems, &bytesAfter, &properties);

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
			if(!window->active_) {
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
					if (mkey_str != WTK_KEY_UNKOWN) {
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
				xic = window->xic_;
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
							is_repeat_keycode = (this->keycode_last_repeat_key_ == xke->keycode);
						}
					}
					this->keycode_last_repeat_key_ = xke->keycode;

					if (KeyDownEvent) {
						KeyDownEvent(window, mkey, is_repeat_keycode, utf8_buf, time);
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

							if (KeyDownEvent) {
								KeyDownEvent(window, mkey, is_repeat_keycode, &utf8_buf[i], time);
							}
						}
					}
				} break;
				case KeyRelease: {
					this->keycode_last_repeat_key_ = -1;
					
					if (KeyUpEvent) {
						KeyUpEvent(window, mkey, time);
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
					if (ButtonDownEvent) {
						ButtonDownEvent(window, WTK_BTN_LEFT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 2: {
					if (ButtonDownEvent) {
						ButtonDownEvent(window, WTK_BTN_MIDDLE, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 3: {
					if (ButtonDownEvent) {
						ButtonDownEvent(window, WTK_BTN_RIGHT, evt->xbutton.x, evt->xbutton.y, time);
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
					if (ButtonUpEvent) {
						ButtonUpEvent(window, WTK_BTN_LEFT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 2: {
					if (ButtonUpEvent) {
						ButtonUpEvent(window, WTK_BTN_MIDDLE, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 3: {
					if (ButtonUpEvent) {
						ButtonUpEvent(window, WTK_BTN_RIGHT, evt->xbutton.x, evt->xbutton.y, time);
					}
				} break;
				case 4: {
					if (WheelEvent) {
						WheelEvent(window, 0, 1, time);
					}
				} break;
				case 5: {
					if (WheelEvent) {
						WheelEvent(window, 0, -1, time);
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
			if(ActivateEvent) {
				ActivateEvent(window, true);
			}
			window->active = true;
		} break;
		
		case FocusOut: {
#if defined(WITH_X11_XINPUT) && defined(X_HAVE_UTF8_STRING)
			if (window->xic) {
				XUnsetICFocus(window->xic);
			}
#endif
			if(ActivateEvent) {
				ActivateEvent(window, false);
			}
			window->active = false;
		} break;
		
		case MotionNotify: {
			if(!window->active) {
				break;
			}
			
			double time = static_cast<double>(evt->xmotion.time) / 1000.0;
			
			if (MouseEvent) {
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
			
			const char *atomName = XGetAtomName(this->display, evt->xclient.message_type);
			if (atomName != nullptr) {
				// ?
			}
		} break;
	}
}
