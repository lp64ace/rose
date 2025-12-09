#ifndef DNA_WINDOWMANAGER_TYPES_H
#define DNA_WINDOWMANAGER_TYPES_H

#include "DNA_ID.h"
#include "DNA_listbase.h"
#include "DNA_screen_types.h"

struct IDProperty;

#ifdef __cplusplus
extern "C" {
#endif

struct rContext;
struct wmEvent;

typedef struct wmWindow {
	struct wmWindow *prev, *next;

	/**
	 * This is the handle of the backend system that we are using,
	 * this handle does not belong to us but is the linking part between TinyWindow and WindowManager.
	 */
	void *handle;
	void *context;

	struct wmWindow *parent;
	struct Screen *screen;
	struct Scene *scene;
	/** Global areas aren't part of the screen, but part of the window directly. */
	struct ScrAreaMap global_areas;

	char view_layer_name[64];

	int posx;
	int posy;
	int sizex;
	int sizey;

	float last_draw;
	float delta_time;
	float fps;

	/** Internal window id, used for matching screens, greater than 0. */
	int winid;

	/** Event state regarding information about the previous events, managed by WindowManager. */
	struct wmEvent *event_state;
	struct wmEvent *event_last_handled;
	/** The time when the key is pressed, used to detect double-click events. */
	double prev_press_event_time;
	/**
	 * The queue of all the WindowManager processed events, these may include non-os related events.
	 * These will be dispatched to handlers, managed by WindowManager.
	 */
	ListBase event_queue;
	ListBase modalhandlers;
	ListBase handlers;
} wmWindow;

typedef struct WindowManager_Runtime {
	struct wmKeyConfig *defaultconf;

	ListBase operators;
	ListBase keymaps;
	ListBase keyconfigs;
} WindowManager_Runtime;

typedef struct WindowManager {
	ID id;

	struct wmWindow *windrawable;

	/**
	 * This is the handle of the backend system that we are using,
	 * this handle does not belong to us but is the linking part between TinyWindow and WindowManager.
	 */
	void *handle;

	/** A list of all #wmWindow links that are allocated. */
	ListBase windows;

	WindowManager_Runtime runtime;
} WindowManager;

typedef struct wmKeyMapItem {
	struct wmKeyMapItem *prev, *next;

	int flag;

	/* operator */

	/** Used to retrieve operator type pointer. */
	char idname[64];
	/** Operator properties, assigned to ptr->data and can be written to a file. */
	struct IDProperty *properties;

	/* event */

	/** Event code itself (#EVT_LEFTCTRLKEY, #LEFTMOUSE etc). */
	int type;
	/** Button state (#KM_ANY, #KM_PRESS, #KM_DBL_CLICK, #KM_PRESS_DRAG, #KM_NOTHING etc). */
	int val;

	/* Modifier keys:
	 * Valid values:
	 * - #KM_ANY
	 * - #KM_NOTHING
	 * - #KM_MOD_HELD (not #KM_PRESS even though the values match).
	 */

	int shift;
	int ctrl;
	int alt;
	/** Also known as "Apple", "Windows-Key" or "Super". */
	int oskey;

	/* runtime */

	int maptype;

	struct PointerRNA *ptr;
} wmKeyMapItem;

/** #wmKeyMapItem->flag */
enum {
	KMI_INACTIVE = (1 << 0),
	KMI_EXPANDED = (1 << 1),
	KMI_USER_MODIFIED = (1 << 2),
	KMI_UPDATE = (1 << 3),
	/**
	 * When set, ignore events with `wmEvent.flag & WM_EVENT_IS_REPEAT` enabled.
	 *
	 * \note this flag isn't cleared when editing/loading the key-map items,
	 * so it may be set in cases which don't make sense (modifier-keys or mouse-motion for example).
	 *
	 * Knowing if an event may repeat is something set at the operating-systems event handling level
	 * so rely on #WM_EVENT_IS_REPEAT being false non keyboard events instead of checking if this
	 * flag makes sense.
	 *
	 * Only used when: `ISKEYBOARD(kmi->type) || (kmi->type == KM_TEXTINPUT)`
	 * as mouse, 3d-mouse, timer... etc never repeat.
	 */
	KMI_REPEAT_IGNORE = (1 << 4),
};

/** #wmKeyMapItem->maptype */
enum {
	KMI_TYPE_KEYBOARD = 0,
	KMI_TYPE_MOUSE = 1,
};

typedef struct wmKeyMap {
	struct wmKeyMap *prev, *next;

	ListBase items;

	/** Global editor keymaps, or for more per space/region. */
	char idname[64];
	char owner[64];

	int spaceid;
	int regionid;

	int (*poll)(struct rContext *C);
} wmKeyMap;

typedef struct wmKeyConfig {
	struct wmKeyConfig *prev, *next;

	/** Unique name. */
	char idname[64];
	/** ID-name of configuration this is derives from, "" if none. */
	char basename[64];

	ListBase keymaps;
} wmKeyConfig;

typedef struct wmOperator {
	struct wmOperator *prev, *next;

	char idname[64];

	struct IDProperty *properties;

	/** Operator type definition from idname. */
	struct wmOperatorType *type;
	/** Custom storage, only while operator runs. */
	void *customdata;

	int flag;

	struct PointerRNA *ptr;
} wmOperator;

enum {
	OP_IS_INVOKE = (1 << 0),
	OP_IS_MODAL_CURSOR_REGION = (1 << 1),
};

typedef enum wmOperatorStatus {
	OPERATOR_RUNNING_MODAL = (1 << 0),
	OPERATOR_CANCELLED = (1 << 1),
	OPERATOR_FINISHED = (1 << 2),
	/** Add this flag if the event should pass through. */
	OPERATOR_PASS_THROUGH = (1 << 3),
	/** In case operator got executed outside WM code (like via file-select). */
	OPERATOR_HANDLED = (1 << 4),
	/**
	 * Used for operators that act indirectly (eg. popup menu).
	 * \note this isn't great design (using operators to trigger UI) avoid where possible.
	 */
	OPERATOR_INTERFACE = (1 << 5),
} wmOperatorStatus;

#ifdef __cplusplus
}
#endif

#endif	// DNA_WINDOWMANAGER_TYPES_H
