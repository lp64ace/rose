#pragma once

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wmEvent {
	struct wmEvent *prev, *next;

	int type;
	int value;
	int flag;
	int xy[2];
	int global[2];
	int local[2];
	wchar_t utf16[2];

	int modifiers;
	
	int custom;
	int customdata_free;
	/**
	 * The #wmEvent::type implies the following #wmEvent::customdata.
	 * 
	 * - #EVT_DROP: used #ListBase of #wmDrag (also #wmEvent::custom == #EVT_DATA_DRAGDROP).
	 */
	void *customdata;

	/** Information about the previous event state. */
	struct {
		int type;
		int value;
		int xy[2];
		int global[2];
		int modifiers;

		int press_type;
		int press_modifiers;
		int press_xy[2];
	} old;
} wmEvent;

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

/** #wmEvent->flag */
enum {
	WM_EVENT_IS_REPEAT = (1 << 1),
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
	WM_HANDLER_DO_FREE = 1 << 0,
	WM_HANDLER_BLOCKING = 1 << 1,
	WM_HANDLER_ACCEPT_DBL_CLICK = 1 << 2,
};

typedef struct wmEventHandler_UI {
	wmEventHandler head;

	wmUIHandlerFunc handle_fn;		 /* callback receiving events */
	wmUIHandlerRemoveFunc remove_fn; /* callback when handler is removed */
	void *user_data;				 /* user data pointer */

	/** Store context for this handler for derived/modal handlers. */
	struct {
		ScrArea *area;
		ARegion *region;
		ARegion *menu;
	} context;
} wmEventHandler_UI;

#define WM_UI_HANDLER_CONTINUE 0
#define WM_UI_HANDLER_BREAK 1

#ifdef __cplusplus
}
#endif
