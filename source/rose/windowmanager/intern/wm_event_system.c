#include "MEM_alloc.h"

#include "DNA_screen_types.h"
#include "DNA_windowmanager_types.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_main.h"
#include "KER_screen.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_event_system.h"
#include "WM_handlers.h"
#include "WM_init_exit.h"
#include "WM_types.h"
#include "WM_window.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_math.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "ED_screen.h"

#include "glib.h"

/** \return The WM enum for key or #EVENT_NONE (which should be ignored). */
static int convert_key(int key);

/* -------------------------------------------------------------------- */
/** \name Event Management
 * \{ */

wmEvent *wm_event_add_ex(wmWindow *win, const wmEvent *event_to_add, const wmEvent *event_to_add_after) {
	wmEvent *evt = MEM_mallocN(sizeof(wmEvent), "wmEvent");

	memcpy(evt, event_to_add, sizeof(wmEvent));

	if (event_to_add_after == NULL) {
		LIB_addtail(&win->event_queue, evt);
	}
	else {
		LIB_insertlinkafter(&win->event_queue, event_to_add_after, evt);
	}

	return evt;
}

wmEvent *wm_event_add(wmWindow *win, const wmEvent *event_to_add) {
	return wm_event_add_ex(win, event_to_add, NULL);
}

static void wm_event_custom_free(struct wmEvent *evt) {
	if (!(evt->customdata && evt->customdata_free)) {
		return;
	}

	switch (evt->custom) {
		default: {
			MEM_freeN(evt->customdata);
		} break;
	}
}

static void wm_event_custom_clear(struct wmEvent *evt) {
	evt->custom = 0;
	evt->customdata = NULL;
	evt->customdata_free = false;
}

static void wm_event_free_last_handled(struct wmWindow *win, struct wmEvent *evt) {
	if (win->event_last_handled) {
		wm_event_free(win->event_last_handled);
	}

	evt->next = evt->prev = NULL;

	wm_event_custom_free(evt);
	wm_event_custom_clear(evt);
	win->event_last_handled = evt;
}

static void wm_event_free_last(struct wmWindow *win) {
	wmEvent *evt = LIB_pophead(&win->event_queue);
	if (evt) {
		wm_event_free(evt);
	}
}

void wm_event_free_all(struct wmWindow *win) {
	wmEvent *evt;
	while (evt = LIB_pophead(&win->event_queue)) {
		wm_event_free(evt);
	}
}

void wm_event_free(struct wmEvent *evt) {
	wm_event_custom_free(evt);

	MEM_freeN(evt);
}

static void wm_event_free_and_remove_from_queue_if_valid(wmEvent *evt) {
	LISTBASE_FOREACH(wmWindowManager *, wm, &G_MAIN->wm) {
		LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
			if (LIB_listbase_contains(&win->event_queue, evt)) {
				LIB_remlink(&win->event_queue, evt);
				wm_event_free(evt);
				return;
			}
		}
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Handler Types
 *
 * General API for different handler types.
 * \{ */

void wm_event_free_handler(struct wmEventHandler *handler) {
	MEM_freeN(handler);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Event Queue Utilities
 *
 * Utilities used by #wm_event_do_handlers.
 * \{ */

static bool wm_event_inside_rect(const wmEvent *event, const rcti *rect) {
	if (LIB_rcti_isect_pt_v(rect, event->xy)) {
		return true;
	}
	return false;
}

static ScrArea *area_event_inside(Context *C, const int xy[2]) {
	wmWindow *win = CTX_wm_window(C);
	Screen *screen = CTX_wm_screen(C);

	if (screen) {
		ED_screen_areas_iter(win, screen, area) {
			if (LIB_rcti_isect_pt_v(&area->totrct, xy)) {
				return area;
			}
		}
	}
	return NULL;
}

static ARegion *region_event_inside(Context *C, const int xy[2]) {
	Screen *screen = CTX_wm_screen(C);
	ScrArea *area = CTX_wm_area(C);

	if (screen && area) {
		LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
			if (LIB_rcti_isect_pt_v(&region->winrct, xy)) {
				return region;
			}
		}
	}
	return NULL;
}

/* \} */

typedef enum eHandlerActionFlag {
	WM_HANDLER_CONTINUE = 0,

	WM_HANDLER_BREAK = 1 << 0,
	WM_HANDLER_HANDLED = 1 << 1,
	WM_HANDLER_MODAL = 1 << 2,
} eHandlerActionFlag;

ENUM_OPERATORS(eHandlerActionFlag, WM_HANDLER_MODAL)

/* -------------------------------------------------------------------- */
/** \name UI Handling
 * \{ */

static eHandlerActionFlag wm_handler_ui_call(struct Context *C, wmEventHandler_UI *handler, const wmEvent *evt) {
	ScrArea *area = CTX_wm_area(C);
	ARegion *region = CTX_wm_region(C);
	ARegion *menu = CTX_wm_menu(C);

	/**
	 * UI code doesn't handle return values - it just always returns break.
	 * to make the #DBL_CLICK conversion work, we just don't send this to UI, except mouse clicks.
	 */
	if (((handler->head.flag & WM_HANDLER_ACCEPT_DBL_CLICK) == 0) && !ISMOUSE_BUTTON(evt->type) && (evt->value == KM_DBL_CLICK)) {
		return WM_HANDLER_CONTINUE;
	}

	/* We set context to where UI handler came from. */
	if (handler->context.area) {
		CTX_wm_area_set(C, handler->context.area);
	}
	if (handler->context.region) {
		CTX_wm_region_set(C, handler->context.region);
	}
	if (handler->context.menu) {
		CTX_wm_menu_set(C, handler->context.menu);
	}

	int retval = handler->handle_fn(C, evt, handler->user_data);

	/* Putting back screen context. */
	if ((retval != WM_UI_HANDLER_BREAK)) {
		CTX_wm_area_set(C, area);
		CTX_wm_region_set(C, region);
		CTX_wm_menu_set(C, menu);
	}
	else {
		/* This special cases is for areas and regions that get removed. */
		CTX_wm_area_set(C, NULL);
		CTX_wm_region_set(C, NULL);
		CTX_wm_menu_set(C, NULL);
	}

	if (retval == WM_UI_HANDLER_BREAK) {
		return WM_HANDLER_BREAK;
	}

	return WM_HANDLER_CONTINUE;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Main Event Queue
 * \{ */

static eHandlerActionFlag wm_handlers_do(struct Context *C, struct wmEvent *evt, struct ListBase *handlers);

static void wm_region_mouse_co(struct Context *C, struct wmEvent *evt) {
	ARegion *region = CTX_wm_region(C);
	if (region) {
		/* Compatibility convention. */
		evt->local[0] = evt->xy[0] - region->winrct.xmin;
		evt->local[1] = evt->xy[1] - region->winrct.ymin;
	}
	else {
		/* These values are invalid (avoid odd behavior by relying on old #wmEvent.mval values). */
		evt->local[0] = -1;
		evt->local[1] = -1;
	}
}

static eHandlerActionFlag wm_event_do_region_handlers(struct Context *C, struct wmEvent *evt, struct ARegion *region) {
	CTX_wm_region_set(C, region);

	wm_region_mouse_co(C, region);

	return wm_handlers_do(C, evt, &region->handlers);
}

static eHandlerActionFlag wm_handlers_do_intern(struct Context *C, struct wmWindow *win, struct wmEvent *evt, struct ListBase *handlers) {
	wmWindowManager *wm = CTX_wm_manager(C);
	eHandlerActionFlag action = WM_HANDLER_CONTINUE;

	if (handlers == NULL) {
		return action;
	}

	LISTBASE_FOREACH_MUTABLE(wmEventHandler *, handler_base, handlers) {
		/* During this loop, UI handlers for nested menus can tag multiple handlers free. */
		if (handler_base->flag & WM_HANDLER_DO_FREE) {
			/* Pass */
		}
		else if (handler_base->poll == NULL || handler_base->poll(CTX_wm_region(C), evt)) {
			if (handler_base->flag & WM_HANDLER_BLOCKING) {
				action |= WM_HANDLER_BREAK;
			}

			if (handler_base->type == WM_HANDLER_TYPE_UI) {
				wmEventHandler_UI *handler = (wmEventHandler_UI *)handler_base;
				ROSE_assert(handler->handle_fn != NULL);
				action |= wm_handler_ui_call(C, handler, evt);
			}
			else {
				ROSE_assert_unreachable();
			}

			if (action & WM_HANDLER_BREAK) {
				break;
			}
		}

		if (CTX_wm_window(C) == NULL) {
			return action;
		}

		if (LIB_listbase_contains(handlers, handler_base)) {
			if (handler_base->flag & WM_HANDLER_DO_FREE) {
				LIB_remlink(handlers, handler_base);
				wm_event_free_handler(handler_base);
			}
		}
	}

	return action;
}

static eHandlerActionFlag wm_handlers_do(struct Context *C, struct wmEvent *evt, struct ListBase *handlers) {
	eHandlerActionFlag action = wm_handlers_do_intern(C, CTX_wm_window(C), evt, handlers);

	wmWindow *win = CTX_wm_window(C);

	if (win == NULL) {
		return action;
	}

	/** Drag events need extra care, this should happen here! */

	return action;
}

static eHandlerActionFlag wm_event_do_handlers_area_regions(struct Context *C, struct wmEvent *event, struct ScrArea *area) {
	ARegion *region_hovered = ED_area_find_region_xy_visual(area, RGN_TYPE_ANY, event->xy);
	if (!region_hovered) {
		return WM_HANDLER_CONTINUE;
	}

	return wm_event_do_region_handlers(C, event, region_hovered);
}

void wm_event_do_handlers(struct Context *C) {
	wmWindowManager *wm = CTX_wm_manager(C);

	LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
		Screen *screen = WM_window_get_active_screen(win);

		ROSE_assert(WM_window_get_active_screen(win));
		ROSE_assert(WM_window_get_active_workspace(win));

		if (screen == NULL) {
			wm_event_free_all(win);
		}
		else {
			/** Any prior updates can happen here! */
		}

		wmEvent *evt;
		while (evt = win->event_queue.first) {
			eHandlerActionFlag action = WM_HANDLER_CONTINUE;

			/* Active screen might change during handlers, update pointer. */
			screen = WM_window_get_active_screen(win);

			CTX_wm_window_set(C, win);

			/* We let modal handlers get active area/region, also wm_paintcursor_test needs it. */
			CTX_wm_area_set(C, area_event_inside(C, evt->xy));
			CTX_wm_region_set(C, region_event_inside(C, evt->xy));

			wm_window_make_drawable(wm, win);

			wm_region_mouse_co(C, evt);

			action |= wm_handlers_do(C, evt, &win->modal_handlers);

			if (CTX_wm_window(C) == NULL) {
				wm_event_free_and_remove_from_queue_if_valid(evt);
				return;
			}

			ED_screen_areas_iter(win, screen, area) {
				if (wm_event_inside_rect(evt, &area->totrct)) {
					CTX_wm_area_set(C, area);

					action |= wm_event_do_handlers_area_regions(C, evt, area);

					if (CTX_wm_window(C) == NULL) {
						wm_event_free_and_remove_from_queue_if_valid(evt);
						return;
					}

					CTX_wm_region_set(C, NULL);

					if ((action & WM_HANDLER_BREAK) == 0) {
						wm_region_mouse_co(C, evt);
						action |= wm_handlers_do(C, evt, &area->handlers);
					}

					CTX_wm_area_set(C, NULL);
				}
			}

			if ((action & WM_HANDLER_BREAK) == 0) {
				/* Also some non-modal handlers need active area/region. */
				CTX_wm_area_set(C, area_event_inside(C, evt->xy));
				CTX_wm_region_set(C, region_event_inside(C, evt->xy));

				wm_region_mouse_co(C, evt);

				action |= wm_handlers_do(C, evt, &win->handlers);

				/* File-read case. */
				if (CTX_wm_window(C) == NULL) {
					wm_event_free_and_remove_from_queue_if_valid(evt);
					return;
				}
			}

			/* Update previous mouse position for following events to use. */
			copy_v2_v2_int(win->eventstate->old.xy, evt->xy);

			LIB_remlink(&win->event_queue, evt);
			wm_event_free_last_handled(win, evt);
		}

		CTX_wm_window_set(C, NULL);
	}
}

/* \} */

static wmWindow *wm_event_cursor_other_window(wmWindowManager *wm, wmWindow *win, wmEvent *evt) {
	if (wm->windows.first == wm->windows.last) {
		return NULL;
	}

	int event_xy[2] = {UNPACK2(evt->xy)};
	if (event_xy[0] < 0 || event_xy[1] < 0 || event_xy[0] > WM_window_pixels_x(win) || event_xy[1] > WM_window_pixels_y(win)) {
		LISTBASE_FOREACH(wmEventHandler *, handler, &win->modal_handlers) {
			if (ELEM(handler->type, WM_HANDLER_TYPE_UI)) {
				return NULL;
			}
		}

		wmWindow *win_other = WM_window_find_under_cursor(win, event_xy, event_xy);
		if (win_other && win_other != win) {
			copy_v2_v2_int(evt->xy, event_xy);
			return win_other;
		}
	}

	return NULL;
}

static wmEvent *wm_event_add_mousemove(wmWindow *win, const wmEvent *evt) {
	wmEvent *event_last = win->event_queue.last;

	if (event_last && event_last->type == MOUSEMOVE) {
		event_last->type = INBETWEEN_MOUSEMOVE;
		event_last->flag = 0;
	}

	wmEvent *event_new = wm_event_add(win, evt);
	if (event_last == NULL) {
		event_last = win->eventstate;
	}

	copy_v2_v2_int(event_new->old.xy, event_last->xy);
	copy_v2_v2_int(event_new->old.global, event_last->global);
	return event_new;
}

static void wm_event_prev_values_set(wmEvent *evt, wmEvent *event_state) {
	evt->old.value = event_state->old.value = event_state->value;
	evt->old.type = event_state->old.type = event_state->type;
}

static bool wm_event_is_double_click(const wmEvent *evt, const uint64_t event_time_ms, const uint64_t event_prev_press_time_ms) {
	if ((evt->type == evt->old.type) && (evt->old.value == KM_RELEASE) && (evt->value == KM_PRESS)) {
		if (ISMOUSE_BUTTON(evt->type) && WM_event_drag_test(evt, evt->old.xy)) {
			/* Pass. */
		}
		else {
			if ((event_time_ms - event_prev_press_time_ms) < 500) {
				return true;
			}
		}
	}
	return false;
}

static void wm_event_prev_click_set(uint64_t event_time_ms, wmEvent *event_state, uint64_t *r_event_state_prev_press_time_ms) {
	event_state->old.press_type = event_state->type;
	event_state->old.press_modifiers = event_state->modifiers;
	event_state->old.press_xy[0] = event_state->xy[0];
	event_state->old.press_xy[1] = event_state->xy[1];
	*r_event_state_prev_press_time_ms = event_time_ms;
}

static void wm_event_state_update_and_click_set_ex(wmEvent *evt, uint64_t event_time_ms, wmEvent *event_state, uint64_t *event_state_prev_press_time_ms_p, const bool is_keyboard, const bool check_double_click) {
	ROSE_assert(ISKEYBOARD_OR_BUTTON(evt->type));
	ROSE_assert(ELEM(evt->value, KM_PRESS, KM_RELEASE));

	const int event_state_flag_mask = WM_EVENT_IS_REPEAT;

	event_state->type = event_state->old.type = evt->type;
	event_state->value = event_state->old.value = evt->value;

	if (is_keyboard) {
		event_state->modifiers = evt->modifiers;
	}
	event_state->flag = (evt->flag & event_state_flag_mask);

	if (check_double_click && wm_event_is_double_click(evt, event_time_ms, *event_state_prev_press_time_ms_p)) {
		evt->value = KM_DBL_CLICK;
	}
	else if (evt->value == KM_PRESS) {
		if ((evt->flag & WM_EVENT_IS_REPEAT) == 0) {
			wm_event_prev_click_set(event_time_ms, event_state, event_state_prev_press_time_ms_p);
		}
	}
}

static void wm_event_state_update_and_click_set(wmEvent *evt, uint64_t event_time_ms, wmEvent *event_state, uint64_t *event_state_prev_press_time_ms_p, int type) {
	const bool is_keyboard = ELEM(type, GLIB_EVT_KEYDOWN, GLIB_EVT_KEYUP);
	const bool check_double_click = true;

	wm_event_state_update_and_click_set_ex(evt, event_time_ms, event_state, event_state_prev_press_time_ms_p, is_keyboard, check_double_click);
}

static bool wm_event_is_same_key_press(const wmEvent *event_a, const wmEvent *event_b) {
	if (event_a->value != KM_PRESS || event_b->value != KM_PRESS) {
		return false;
	}

	if (event_a->modifiers != event_b->modifiers || event_a->type != event_b->type) {
		return false;
	}

	return true;
}

static bool wm_event_is_ignorable_key_press(const wmWindow *win, const wmEvent *evt) {
	if (LIB_listbase_is_empty(&win->event_queue)) {
		return false;
	}

	if ((evt->flag & WM_EVENT_IS_REPEAT) == 0) {
		return false;
	}

	const wmEvent *last_event = win->event_queue.last;

	return wm_event_is_same_key_press(last_event, evt);
}

void wm_event_add_ghostevent(wmWindowManager *wm, wmWindow *win, const int type, const void *customdata_, const uint64_t event_time_ms) {
	wmEvent evt, *event_state = win->eventstate;
	uint64_t *event_state_prev_press_time_ms_p = &win->eventstate_prev_press_time_ms;

	memcpy(&evt, event_state, sizeof(wmEvent));
	evt.flag = 0;

	evt.old.type = evt.type;
	evt.old.value = evt.value;

	if ((wm->winactive != win) && (wm->winactive && wm->winactive->eventstate)) {
		evt.modifiers = wm->winactive->eventstate->modifiers;
	}

	if ((event_state->type || event_state->value) && !(ISKEYBOARD_OR_BUTTON(event_state->type) || (event_state->type == EVENT_NONE))) {
		ROSE_assert_unreachable();
	}
	if ((event_state->old.type || event_state->old.value) && !(ISKEYBOARD_OR_BUTTON(event_state->old.type) || (event_state->old.type == EVENT_NONE))) {
		ROSE_assert_unreachable();
	}

	switch (type) {
		case GLIB_EVT_MOUSEMOVE: {
			const EventMouse *customdata = (const struct EventMouse *)(customdata_);

			evt.xy[0] = customdata->x;
			evt.xy[1] = customdata->y;
			wm_cursor_position_to_ghost_client_coords(win, &evt.xy[0], &evt.xy[1]);

			copy_v2_v2_int(&evt.global, &evt.xy);
			wm_cursor_position_to_ghost_screen_coords(win, &evt.global[0], &evt.global[1]);

			evt.type = MOUSEMOVE;
			evt.value = KM_NOTHING;
			{
				wmEvent *event_new = wm_event_add_mousemove(win, &evt);
				copy_v2_v2_int(event_state->xy, event_new->xy);
				copy_v2_v2_int(event_state->old.xy, event_new->xy);
			}

			wmWindow *win_other = wm_event_cursor_other_window(wm, win, &evt);
			if (win_other) {
				wmEvent evt_other;

				memcpy(&evt_other, win_other->eventstate, sizeof(wmEvent));

				evt_other.modifiers = evt.modifiers;
				evt_other.old.type = evt_other.type;
				evt_other.old.value = evt_other.value;

				copy_v2_v2_int(evt_other.xy, evt.xy);
				evt_other.type = MOUSEMOVE;
				evt_other.value = KM_NOTHING;
				{
					wmEvent *event_new = wm_event_add_mousemove(win_other, &evt_other);
					copy_v2_v2_int(win_other->eventstate->xy, evt_other.xy);
				}
			}
		} break;
		case GLIB_EVT_LMOUSEDOWN:
		case GLIB_EVT_MMOUSEDOWN:
		case GLIB_EVT_RMOUSEDOWN:
		case GLIB_EVT_LMOUSEUP:
		case GLIB_EVT_MMOUSEUP:
		case GLIB_EVT_RMOUSEUP: {
			const EventMouse *customdata = (const struct EventMouse *)(customdata_);

			evt.value = (ELEM(type, GLIB_EVT_LMOUSEDOWN, GLIB_EVT_MMOUSEDOWN, GLIB_EVT_RMOUSEDOWN)) ? KM_PRESS : KM_RELEASE;

			if (type == GLIB_EVT_LMOUSEDOWN || type == GLIB_EVT_LMOUSEUP) {
				evt.type = LEFTMOUSE;
			}
			else if (type == GLIB_EVT_MMOUSEDOWN || type == GLIB_EVT_MMOUSEUP) {
				evt.type = MIDDLEMOUSE;
			}
			else if (type == GLIB_EVT_RMOUSEDOWN || type == GLIB_EVT_RMOUSEUP) {
				evt.type = RIGHTMOUSE;
			}

			evt.xy[0] = customdata->x;
			evt.xy[1] = customdata->y;
			wm_cursor_position_to_ghost_client_coords(win, &evt.xy[0], &evt.xy[1]);

			copy_v2_v2_int(&evt.global, &evt.xy);
			wm_cursor_position_to_ghost_screen_coords(win, &evt.global[0], &evt.global[1]);

			wm_event_state_update_and_click_set(&evt, event_time_ms, event_state, event_state_prev_press_time_ms_p, type);

			wmWindow *win_other = wm_event_cursor_other_window(wm, win, &evt);
			if (win_other) {
				wmEvent evt_other;

				memcpy(&evt_other, win_other->eventstate, sizeof(wmEvent));

				evt_other.modifiers = evt.modifiers;
				evt_other.old.type = evt_other.type;
				evt_other.old.value = evt_other.value;

				copy_v2_v2_int(evt_other.xy, evt.xy);
				evt_other.type = MOUSEMOVE;
				evt_other.value = KM_NOTHING;

				wm_event_add(win_other, &evt_other);
			}
			else {
				wm_event_add(win, &evt);
			}
		} break;
		case GLIB_EVT_KEYDOWN:
		case GLIB_EVT_KEYUP: {
			const EventKey *customdata = (const struct EventKey *)(customdata_);

			evt.type = convert_key(customdata->key);
			if (evt.type == EVENT_NONE) {
				break;
			}

			memcpy(evt.utf8, customdata->utf8, sizeof(evt.utf8));
			if (customdata->repeat) {
				evt.flag |= WM_EVENT_IS_REPEAT;
			}
			evt.value = (type == GLIB_EVT_KEYDOWN) ? KM_PRESS : KM_RELEASE;

			if (type == GLIB_EVT_KEYUP) {
				MEMZERO(evt.utf8);
			}

			switch (evt.type) {
				case EVT_LEFTSHIFTKEY:
				case EVT_RIGHTSHIFTKEY: {
					SET_FLAG_FROM_TEST(evt.modifiers, (evt.value == KM_PRESS), KM_SHIFT);
				} break;
				case EVT_LEFTCTRLKEY:
				case EVT_RIGHTCTRLKEY: {
					SET_FLAG_FROM_TEST(evt.modifiers, (evt.value == KM_PRESS), KM_CTRL);
				} break;
				case EVT_LEFTALTKEY:
				case EVT_RIGHTALTKEY: {
					SET_FLAG_FROM_TEST(evt.modifiers, (evt.value == KM_PRESS), KM_ALT);
				} break;
				case EVT_OSKEY: {
					SET_FLAG_FROM_TEST(evt.modifiers, (evt.value == KM_PRESS), KM_OSKEY);

				} break;
			}

			wm_event_state_update_and_click_set(&evt, event_time_ms, event_state, event_state_prev_press_time_ms_p, type);

			if (!wm_event_is_ignorable_key_press(win, &evt)) {
				wm_event_add(win, &evt);
			}
		} break;
		case GLIB_EVT_MOUSEWHEEL: {
			const EventMouse *customdata = (const struct EventMouse *)(customdata_);

			if (customdata->wheel > 0) {
				evt.type = WHEELUPMOUSE;
			}
			else {
				evt.type = WHEELDOWNMOUSE;
			}
			evt.value = KM_PRESS;

			wm_event_add(win, &evt);
		} break;
	}
}

/* -------------------------------------------------------------------- */
/** \name Handlers
 * \{ */

static wmEventHandler_UI *wm_ui_handler_find(ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn, void *user_data) {
	LISTBASE_FOREACH(wmEventHandler *, handler_base, handlers) {
		if (handler_base->type == WM_HANDLER_TYPE_UI) {
			wmEventHandler_UI *handler = (wmEventHandler_UI *)handler_base;
			if ((handler->handle_fn == handle_fn) && (handler->remove_fn == remove_fn) && (handler->user_data == user_data)) {
				return handler;
			}
		}
	}
	return NULL;
}

wmEventHandler_UI *WM_event_add_ui_handler(const Context *C, ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn, void *user_data, int flag) {
	wmEventHandler_UI *handler = MEM_callocN(sizeof(wmEventHandler_UI), "wmEventHandler_UI");

	handler->head.type = WM_HANDLER_TYPE_UI;
	handler->handle_fn = handle_fn;
	handler->remove_fn = remove_fn;
	handler->user_data = user_data;
	if (C) {
		handler->context.area = CTX_wm_area(C);
		handler->context.region = CTX_wm_region(C);
		handler->context.menu = CTX_wm_menu(C);
	}
	else {
		handler->context.area = NULL;
		handler->context.region = NULL;
		handler->context.menu = NULL;
	}

	ROSE_assert((flag & WM_HANDLER_DO_FREE) == 0);
	handler->head.flag = flag;

	LIB_addhead(handlers, handler);

	return handler;
}

struct wmEventHandler_UI *WM_event_ensure_ui_handler(const struct Context *C, ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn, void *user_data, int flag) {
	wmEventHandler_UI *handler = wm_ui_handler_find(handlers, handle_fn, remove_fn, user_data);

	if (handler == NULL) {
		handler = WM_event_add_ui_handler(C, handlers, handle_fn, remove_fn, user_data, flag);
	}

	if (C) {
		handler->context.area = CTX_wm_area(C);
		handler->context.region = CTX_wm_region(C);
		handler->context.menu = CTX_wm_menu(C);
	}
	else {
		handler->context.area = NULL;
		handler->context.region = NULL;
		handler->context.menu = NULL;
	}

	return handler;
}

void WM_event_remove_ui_handler(ListBase *handlers, wmUIHandlerFunc handle_fn, wmUIHandlerRemoveFunc remove_fn, void *user_data, const bool postpone) {
	LISTBASE_FOREACH(wmEventHandler *, handler_base, handlers) {
		if (handler_base->type == WM_HANDLER_TYPE_UI) {
			wmEventHandler_UI *handler = (wmEventHandler_UI *)handler_base;
			if ((handler->handle_fn == handle_fn) && (handler->remove_fn == remove_fn) && (handler->user_data == user_data)) {
				/* Handlers will be freed in #wm_handlers_do(). */
				if (postpone) {
					handler->head.flag |= WM_HANDLER_DO_FREE;
				}
				else {
					LIB_remlink(handlers, handler);
					wm_event_free_handler(&handler->head);
				}
				break;
			}
		}
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Ugly Stuff
 * \{ */

static int convert_key(int key) {
	if (key >= GLIB_KEY_A && key <= GLIB_KEY_Z) {
		return (EVT_AKEY + (key - GLIB_KEY_A));
	}
	if (key >= GLIB_KEY_0 && key <= GLIB_KEY_9) {
		return (EVT_ZEROKEY + (key - GLIB_KEY_0));
	}
	if (key >= GLIB_KEY_NUMPAD_0 && key <= GLIB_KEY_NUMPAD_9) {
		return (EVT_PAD0 + (key - GLIB_KEY_NUMPAD_0));
	}
	if (key >= GLIB_KEY_F1 && key <= GLIB_KEY_F24) {
		return (EVT_F1KEY + (key - GLIB_KEY_F1));
	}

	switch (key) {
		case GLIB_KEY_BACKSPACE:
			return EVT_BACKSPACEKEY;
		case GLIB_KEY_TAB:
			return EVT_TABKEY;
		case GLIB_KEY_CLEAR:
			return EVENT_NONE;
		case GLIB_KEY_ENTER:
			return EVT_RETKEY;

		case GLIB_KEY_ESC:
			return EVT_ESCKEY;
		case GLIB_KEY_SPACE:
			return EVT_SPACEKEY;
		case GLIB_KEY_QUOTE:
			return EVT_QUOTEKEY;
		case GLIB_KEY_COMMA:
			return EVT_COMMAKEY;
		case GLIB_KEY_MINUS:
			return EVT_MINUSKEY;
		case GLIB_KEY_PLUS:
			return EVT_PLUSKEY;
		case GLIB_KEY_PERIOD:
			return EVT_PERIODKEY;
		case GLIB_KEY_SLASH:
			return EVT_SLASHKEY;

		case GLIB_KEY_SEMICOLON:
			return EVT_SEMICOLONKEY;
		case GLIB_KEY_EQUAL:
			return EVT_EQUALKEY;

		case GLIB_KEY_LEFT_BRACKET:
			return EVT_LEFTBRACKETKEY;
		case GLIB_KEY_RIGHT_BRACKET:
			return EVT_RIGHTBRACKETKEY;
		case GLIB_KEY_BACKSLASH:
			return EVT_BACKSLASHKEY;
		case GLIB_KEY_ACCENTGRAVE:
			return EVT_ACCENTGRAVEKEY;

		case GLIB_KEY_LEFT_SHIFT:
			return EVT_LEFTSHIFTKEY;
		case GLIB_KEY_RIGHT_SHIFT:
			return EVT_RIGHTSHIFTKEY;
		case GLIB_KEY_LEFT_CTRL:
			return EVT_LEFTCTRLKEY;
		case GLIB_KEY_RIGHT_CTRL:
			return EVT_RIGHTCTRLKEY;
		case GLIB_KEY_LEFT_OS:
		case GLIB_KEY_RIGHT_OS:
			return EVT_OSKEY;
		case GLIB_KEY_LEFT_ALT:
			return EVT_LEFTALTKEY;
		case GLIB_KEY_RIGHT_ALT:
			return EVT_RIGHTALTKEY;
		case GLIB_KEY_APP:
			return EVT_APPKEY;

		case GLIB_KEY_CAPSLOCK:
			return EVT_CAPSLOCKKEY;
		case GLIB_KEY_NUMLOCK:
			return EVENT_NONE;
		case GLIB_KEY_SCRLOCK:
			return EVENT_NONE;

		case GLIB_KEY_LEFT:
			return EVT_LEFTARROWKEY;
		case GLIB_KEY_RIGHT:
			return EVT_RIGHTARROWKEY;
		case GLIB_KEY_UP:
			return EVT_UPARROWKEY;
		case GLIB_KEY_DOWN:
			return EVT_DOWNARROWKEY;

		case GLIB_KEY_PRTSCN:
			return EVENT_NONE;
		case GLIB_KEY_PAUSE:
			return EVT_PAUSEKEY;

		case GLIB_KEY_INSERT:
			return EVT_INSERTKEY;
		case GLIB_KEY_DELETE:
			return EVT_DELKEY;
		case GLIB_KEY_HOME:
			return EVT_HOMEKEY;
		case GLIB_KEY_END:
			return EVT_ENDKEY;
		case GLIB_KEY_PAGEUP:
			return EVT_PAGEUPKEY;
		case GLIB_KEY_PAGEDOWN:
			return EVT_PAGEDOWNKEY;

		case GLIB_KEY_NUMPAD_PERIOD:
			return EVT_PADPERIOD;
		case GLIB_KEY_NUMPAD_ENTER:
			return EVT_PADENTER;
		case GLIB_KEY_NUMPAD_PLUS:
			return EVT_PADPLUSKEY;
		case GLIB_KEY_NUMPAD_MINUS:
			return EVT_PADMINUS;
		case GLIB_KEY_NUMPAD_ASTERISK:
			return EVT_PADASTERKEY;
		case GLIB_KEY_NUMPAD_SLASH:
			return EVT_PADSLASHKEY;

		case GLIB_KEY_GRLESS:
			return EVT_GRLESSKEY;

		case GLIB_KEY_MEDIA_PLAY:
			return EVT_MEDIAPLAY;
		case GLIB_KEY_MEDIA_STOP:
			return EVT_MEDIASTOP;
		case GLIB_KEY_MEDIA_FIRST:
			return EVT_MEDIAFIRST;
		case GLIB_KEY_MEDIA_LAST:
			return EVT_MEDIALAST;

		case GLIB_KEY_UNKOWN:
			return EVT_UNKNOWNKEY;
	}

	return EVENT_NONE;
}

/* \} */
