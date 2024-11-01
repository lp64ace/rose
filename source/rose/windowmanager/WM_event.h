#ifndef WM_EVENT_H
#define WM_EVENT_H

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct WindowManager;
struct wmWindow;

enum {
	EVENT_NONE = 0x0000,

/* Minimum mouse value (inclusive). */
#define _EVT_MOUSE_MIN 0x0001

	/* MOUSE: 0x000x, 0x001x */
	LEFTMOUSE = 0x0001,
	MIDDLEMOUSE = 0x0002,
	RIGHTMOUSE = 0x0003,
	MOUSEMOVE = 0x0004,
	/* Extra mouse buttons */
	BUTTON4MOUSE = 0x0007,
	BUTTON5MOUSE = 0x0008,
	/* More mouse buttons - can't use 9 and 10 here (wheel) */
	BUTTON6MOUSE = 0x0012,
	BUTTON7MOUSE = 0x0013,
	/* Extra trackpad gestures (check #WM_EVENT_IS_CONSECUTIVE to detect motion events). */
	MOUSEPAN = 0x000e,
	MOUSEZOOM = 0x000f,
	MOUSEROTATE = 0x0010,
	MOUSESMARTZOOM = 0x0017,

	/* defaults from ghost */
	WHEELUPMOUSE = 0x000a,
	WHEELDOWNMOUSE = 0x000b,
	/* mapped with userdef */
	WHEELINMOUSE = 0x000c,
	WHEELOUTMOUSE = 0x000d,
	/* Successive MOUSEMOVE's are converted to this, so we can easily
	 * ignore all but the most recent MOUSEMOVE (for better performance),
	 * paint and drawing tools however will want to handle these. */
	INBETWEEN_MOUSEMOVE = 0x0011,

/* Maximum mouse value (inclusive). */
#define _EVT_MOUSE_MAX 0x0011

/* Minimum keyboard value (inclusive). */
#define _EVT_KEYBOARD_MIN 0x0020

	EVT_ZEROKEY = 0x0030,  /* '0' (48). */
	EVT_ONEKEY = 0x0031,   /* '1' (49). */
	EVT_TWOKEY = 0x0032,   /* '2' (50). */
	EVT_THREEKEY = 0x0033, /* '3' (51). */
	EVT_FOURKEY = 0x0034,  /* '4' (52). */
	EVT_FIVEKEY = 0x0035,  /* '5' (53). */
	EVT_SIXKEY = 0x0036,   /* '6' (54). */
	EVT_SEVENKEY = 0x0037, /* '7' (55). */
	EVT_EIGHTKEY = 0x0038, /* '8' (56). */
	EVT_NINEKEY = 0x0039,  /* '9' (57). */

	EVT_AKEY = 0x0061, /* 'a' (97). */
	EVT_BKEY = 0x0062, /* 'b' (98). */
	EVT_CKEY = 0x0063, /* 'c' (99). */
	EVT_DKEY = 0x0064, /* 'd' (100). */
	EVT_EKEY = 0x0065, /* 'e' (101). */
	EVT_FKEY = 0x0066, /* 'f' (102). */
	EVT_GKEY = 0x0067, /* 'g' (103). */
	EVT_HKEY = 0x0068, /* 'h' (104). */
	EVT_IKEY = 0x0069, /* 'i' (105). */
	EVT_JKEY = 0x006a, /* 'j' (106). */
	EVT_KKEY = 0x006b, /* 'k' (107). */
	EVT_LKEY = 0x006c, /* 'l' (108). */
	EVT_MKEY = 0x006d, /* 'm' (109). */
	EVT_NKEY = 0x006e, /* 'n' (110). */
	EVT_OKEY = 0x006f, /* 'o' (111). */
	EVT_PKEY = 0x0070, /* 'p' (112). */
	EVT_QKEY = 0x0071, /* 'q' (113). */
	EVT_RKEY = 0x0072, /* 'r' (114). */
	EVT_SKEY = 0x0073, /* 's' (115). */
	EVT_TKEY = 0x0074, /* 't' (116). */
	EVT_UKEY = 0x0075, /* 'u' (117). */
	EVT_VKEY = 0x0076, /* 'v' (118). */
	EVT_WKEY = 0x0077, /* 'w' (119). */
	EVT_XKEY = 0x0078, /* 'x' (120). */
	EVT_YKEY = 0x0079, /* 'y' (121). */
	EVT_ZKEY = 0x007a, /* 'z' (122). */

	EVT_LEFTARROWKEY = 0x0089,	/* 137 */
	EVT_DOWNARROWKEY = 0x008a,	/* 138 */
	EVT_RIGHTARROWKEY = 0x008b, /* 139 */
	EVT_UPARROWKEY = 0x008c,	/* 140 */

	EVT_PAD0 = 0x0096, /* 150 */
	EVT_PAD1 = 0x0097, /* 151 */
	EVT_PAD2 = 0x0098, /* 152 */
	EVT_PAD3 = 0x0099, /* 153 */
	EVT_PAD4 = 0x009a, /* 154 */
	EVT_PAD5 = 0x009b, /* 155 */
	EVT_PAD6 = 0x009c, /* 156 */
	EVT_PAD7 = 0x009d, /* 157 */
	EVT_PAD8 = 0x009e, /* 158 */
	EVT_PAD9 = 0x009f, /* 159 */
	/* Key-pad keys. */
	EVT_PADASTERKEY = 0x00a0, /* 160 */
	EVT_PADSLASHKEY = 0x00a1, /* 161 */
	EVT_PADMINUS = 0x00a2,	  /* 162 */
	EVT_PADENTER = 0x00a3,	  /* 163 */
	EVT_PADPLUSKEY = 0x00a4,  /* 164 */

	EVT_PAUSEKEY = 0x00a5,	  /* 165 */
	EVT_INSERTKEY = 0x00a6,	  /* 166 */
	EVT_HOMEKEY = 0x00a7,	  /* 167 */
	EVT_PAGEUPKEY = 0x00a8,	  /* 168 */
	EVT_PAGEDOWNKEY = 0x00a9, /* 169 */
	EVT_ENDKEY = 0x00aa,	  /* 170 */
	/* Note that 'PADPERIOD' is defined out-of-order. */
	EVT_UNKNOWNKEY = 0x00ab, /* 171 */
	EVT_OSKEY = 0x00ac,		 /* 172 */
	EVT_GRLESSKEY = 0x00ad,	 /* 173 */
	/* Media keys. */
	EVT_MEDIAPLAY = 0x00ae,	 /* 174 */
	EVT_MEDIASTOP = 0x00af,	 /* 175 */
	EVT_MEDIAFIRST = 0x00b0, /* 176 */
	EVT_MEDIALAST = 0x00b1,	 /* 177 */
	/* Menu/App key. */
	EVT_APPKEY = 0x00b2, /* 178 */

	EVT_PADPERIOD = 0x00c7, /* 199 */

	EVT_CAPSLOCKKEY = 0x00d3, /* 211 */

	/* Modifier keys. */
	EVT_LEFTCTRLKEY = 0x00d4,	/* 212 */
	EVT_LEFTALTKEY = 0x00d5,	/* 213 */
	EVT_RIGHTALTKEY = 0x00d6,	/* 214 */
	EVT_RIGHTCTRLKEY = 0x00d7,	/* 215 */
	EVT_RIGHTSHIFTKEY = 0x00d8, /* 216 */
	EVT_LEFTSHIFTKEY = 0x00d9,	/* 217 */
	/* Special characters. */
	EVT_ESCKEY = 0x00da,		  /* 218 */
	EVT_TABKEY = 0x00db,		  /* 219 */
	EVT_RETKEY = 0x00dc,		  /* 220 */
	EVT_SPACEKEY = 0x00dd,		  /* 221 */
	EVT_LINEFEEDKEY = 0x00de,	  /* 222 */
	EVT_BACKSPACEKEY = 0x00df,	  /* 223 */
	EVT_DELKEY = 0x00e0,		  /* 224 */
	EVT_SEMICOLONKEY = 0x00e1,	  /* 225 */
	EVT_PERIODKEY = 0x00e2,		  /* 226 */
	EVT_COMMAKEY = 0x00e3,		  /* 227 */
	EVT_QUOTEKEY = 0x00e4,		  /* 228 */
	EVT_ACCENTGRAVEKEY = 0x00e5,  /* 229 */
	EVT_MINUSKEY = 0x00e6,		  /* 230 */
	EVT_PLUSKEY = 0x00e7,		  /* 231 */
	EVT_SLASHKEY = 0x00e8,		  /* 232 */
	EVT_BACKSLASHKEY = 0x00e9,	  /* 233 */
	EVT_EQUALKEY = 0x00ea,		  /* 234 */
	EVT_LEFTBRACKETKEY = 0x00eb,  /* 235 */
	EVT_RIGHTBRACKETKEY = 0x00ec, /* 236 */

/* Maximum keyboard value (inclusive). */
#define _EVT_KEYBOARD_MAX 0x00ff

	EVT_F1KEY = 0x012c,	 /* 300 */
	EVT_F2KEY = 0x012d,	 /* 301 */
	EVT_F3KEY = 0x012e,	 /* 302 */
	EVT_F4KEY = 0x012f,	 /* 303 */
	EVT_F5KEY = 0x0130,	 /* 304 */
	EVT_F6KEY = 0x0131,	 /* 305 */
	EVT_F7KEY = 0x0132,	 /* 306 */
	EVT_F8KEY = 0x0133,	 /* 307 */
	EVT_F9KEY = 0x0134,	 /* 308 */
	EVT_F10KEY = 0x0135, /* 309 */
	EVT_F11KEY = 0x0136, /* 310 */
	EVT_F12KEY = 0x0137, /* 311 */
	EVT_F13KEY = 0x0138, /* 312 */
	EVT_F14KEY = 0x0139, /* 313 */
	EVT_F15KEY = 0x013a, /* 314 */
	EVT_F16KEY = 0x013b, /* 315 */
	EVT_F17KEY = 0x013c, /* 316 */
	EVT_F18KEY = 0x013d, /* 317 */
	EVT_F19KEY = 0x013e, /* 318 */
	EVT_F20KEY = 0x013f, /* 319 */
	EVT_F21KEY = 0x0140, /* 320 */
	EVT_F22KEY = 0x0141, /* 321 */
	EVT_F23KEY = 0x0142, /* 322 */
	EVT_F24KEY = 0x0143, /* 323 */
};

/* clang-format off */

/** Test whether the event is a key on the keyboard (including modifier keys). */
#define ISKEYBOARD(event_type) ((_EVT_KEYBOARD_MIN <= (event_type) && (event_type) <= _EVT_KEYBOARD_MAX) || (EVT_F1KEY <= (event_type) && (event_type) <= EVT_F24KEY))

/**
 * Test whether the event is any kind:
 * #ISMOUSE_MOTION, #ISMOUSE_BUTTON, #ISMOUSE_WHEEL & #ISMOUSE_GESTURE.
 *
 * \note It's best to use more specific check if possible as mixing motion/buttons/gestures
 * is very broad and not necessarily obvious which kinds of events are important.
 */
#define ISMOUSE(event_type) (_EVT_MOUSE_MIN <= (event_type) && (event_type) <= _EVT_MOUSE_MAX)
/** Test whether the event is a mouse button (excluding mouse-wheel). */
#define ISMOUSE_MOTION(event_type) ELEM(event_type, MOUSEMOVE, INBETWEEN_MOUSEMOVE)
/** Test whether the event is a mouse button (excluding mouse-wheel). */
#define ISMOUSE_BUTTON(event_type) (ELEM(event_type, LEFTMOUSE, MIDDLEMOUSE, RIGHTMOUSE, BUTTON4MOUSE, BUTTON5MOUSE, BUTTON6MOUSE, BUTTON7MOUSE))
/** Test whether the event is a mouse wheel. */
#define ISMOUSE_WHEEL(event_type) (WHEELUPMOUSE <= (event_type) && (event_type) <= WHEELOUTMOUSE)
/** Test whether the event is a mouse (trackpad) gesture. */
#define ISMOUSE_GESTURE(event_type) (MOUSEPAN <= (event_type) && (event_type) <= MOUSESMARTZOOM)

#define ISKEYBOARD_OR_BUTTON(event_type) (ISMOUSE_BUTTON(event_type) || ISKEYBOARD(event_type))

/* clang-format on */

/* -------------------------------------------------------------------- */
/** \name Exposed Methods
 * \{ */

void wm_event_free_all(struct wmWindow *window);

void wm_event_add_tiny_window_mouse_button(struct WindowManager *wm, struct wmWindow *window, int type, int button, int x, int y, double time);
void wm_event_add_tiny_window_key(struct WindowManager *wm, struct wmWindow *window, int type, int key, bool repeat, char utf8[4], double event_time);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // WM_EVENT_H
