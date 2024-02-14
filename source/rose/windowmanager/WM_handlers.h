#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wmEvent {
	struct wmEvent *prev, *next;

	int type;
	int value;
	int x;
	int y;
	int localx;
	int localy;
	wchar_t utf16[3];
	
	int modifiers;
	
	/** Information about the previous event state. */
	struct {
		int type;
		int value;
		int x;
		int y;
		
		int modifiers;
	} old;
} wmEvent;

/** #wmEvent->type */
enum {
	EVENT_NONE = 0x0000,

#define _EVT_MOUSE_MIN LEFTMOUSE

	LEFTMOUSE,
	MIDDLEMOUSE,
	RIGHTMOUSE,
	MOUSEMOVE,
	BUTTON4MOUSE,
	BUTTON5MOUSE,
	BUTTON6MOUSE,
	BUTTON7MOUSE,

	WHEELUPMOUSE,
	WHEELDOWNMOUSE,

	INBETWEEN_MOUSEMOVE,

#define _EVT_MOUSE_MAX INBETWEEN_MOUSEMOVE
};

/** #wmEvent->value */
enum {
	KM_ANY = -1,
	KM_NOTHING = 0,
	KM_PRESS = 1,
	KM_RELEASE = 2,
	KM_CLICK = 3,
	KM_DBL_CLICK = 4,
	/**
	 * \note The cursor location at the point dragging starts is set to #wmEvent.prev.[xy]
	 * some operators such as box selection should use this location instead of #wmEvent.xy.
	 */
	KM_CLICK_DRAG = 5,
};

/** #wmEvent->modifiers */
enum {
	KM_SHIFT = (1 << 0),
	KM_CTRL = (1 << 1),
	KM_ALT = (1 << 2),
	KM_OSKEY = (1 << 3),
};

typedef bool (*EventHandlerPoll)(const struct ARegion *region, const struct wmEvent *event);

typedef struct wmEventHandler {
	struct wmEventHandler *prev, *next;
	
	int type;
	int flag;
	
	EventHandlerPoll poll;
} wmEventHandler;

/** #wmEventHandler->type */
enum {
	WM_HANDLER_TYPE_UI,
};

/** #wmEventHandler->flag */
enum {
};

#ifdef __cplusplus
}
#endif
