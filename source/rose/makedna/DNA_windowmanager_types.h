#ifndef DNA_WINDOWMANAGER_TYPES_H
#define DNA_WINDOWMANAGER_TYPES_H

#include "DNA_ID.h"
#include "DNA_listbase.h"
#include "DNA_screen_types.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	/** Global areas aren't part of the screen, but part of the window directly. */
	struct ScrAreaMap global_areas;

	int posx;
	int posy;
	unsigned int sizex;
	unsigned int sizey;

	double last_draw;
	double delta_time;
	double fps;

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
} WindowManager;

#ifdef __cplusplus
}
#endif

#endif	// DNA_WINDOWMANAGER_TYPES_H
