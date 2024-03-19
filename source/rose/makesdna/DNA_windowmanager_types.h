#pragma once

#include "DNA_ID.h"
#include "DNA_listbase.h"

#include "DNA_screen_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wmWindowManager {
	ID id;

	struct wmWindow *winactive;
	struct wmWindow *drawable;

	ListBase windows;
} wmWindowManager;

typedef struct wmWindow {
	struct wmWindow *prev, *next;

	struct GWindow *gwin;
	struct GPUContext *gpuctx;

	struct wmWindow *parent;
	
	struct WorkSpaceInstanceHook *workspace_hook;
	
	/** Global areas aren't part of the screen, but part of the widow directly. */
	ScrAreaMap global_areas;
	
	int winid;
	
	int posx;
	int posy;
	int width;
	int height;
	
	/**
	 * Storage for event system.
	 *
	 * For the most part this is storage for `wmEvent.xy` and `wmEvent.modifiers`
	 * newly added key/button events copy the cursor location and modifiers state stored here.
	 *
	 * It's also convenient at times to be able to pass this as if it's a regular event.
	 * 
	 * - This is not simply the current event being handled.
	 *   The type and value is always set to the last press/release events
	 *   otherwise cursor motion would always clear these values.
	 *
	 * - The value of `eventstate->modifiers` is set from the last pressed/released modifier key.
	 *   This has the down side that the modifier value will be incorrect if users hold both left/right
	 *   modifiers then release one.
	 */
	struct wmEvent *eventstate;
	
	/**
	 * Keep the last handled event in `event_queue` here (owned and must be freed).
	 *
	 * \warning This must only be used for event queue logic.
	 * User interactions should use `eventstate` instead (if the event isn't passed to the function).
	 */
	struct wmEvent *event_last_handled;
	
	/** The time when the key is pressed in milliseconds, used to detect double-click events. */
	uint64_t eventstate_prev_press_time_ms;
	
	/** All events #wmEvent (ghost level events were handled). */
	ListBase event_queue;
	/** Window & Screen handlers, handled last. */
	ListBase handlers;
	/** Priority handlers, handled first */
	ListBase modal_handlers;
} wmWindow;

#ifdef __cplusplus
}
#endif
