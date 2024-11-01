#include "MEM_guardedalloc.h"

#include "LIB_math_vector.h"

#include "WM_draw.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_main.h"

#include <tiny_window.h>

/* -------------------------------------------------------------------- */
/** \name Event Management
 * \{ */

wmEvent *wm_event_add_ex(wmWindow *win, const wmEvent *event_to_add, wmEvent *event_to_add_after);
wmEvent *wm_event_add(wmWindow *win, const wmEvent *event_to_add);

void wm_event_update_last_handled(wmWindow *win, wmEvent *evt);
void wm_event_free_last(wmWindow *win);
void wm_event_free_all(wmWindow *win);
void wm_event_free(wmEvent *evt);

wmEvent *wm_event_add_ex(wmWindow *win, const wmEvent *event_to_add, wmEvent *event_to_add_after) {
	wmEvent *evt = MEM_mallocN(sizeof(wmEvent), "wmEvent");

	memcpy(evt, event_to_add, sizeof(wmEvent));

	if(event_to_add_after == NULL) {
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

void wm_event_update_last_handled(wmWindow *win, wmEvent *evt) {
	if(win->event_last_handled) {
		wm_event_free(win->event_last_handled);
	}
	
	evt->next = evt->prev = NULL;
	win->event_last_handled = evt;
}

void wm_event_free_last(wmWindow *win) {
	wmEvent *evt = LIB_pophead(&win->event_queue);
	if(evt) {
		wm_event_free(evt);
	}
}
void wm_event_free_all(wmWindow *win) {
	for (wmEvent *evt; evt = LIB_pophead(&win->event_queue);) {
		wm_event_free(evt);
	}
	if (win->event_last_handled) {
		wm_event_free(win->event_last_handled);
	}
}
void wm_event_free(wmEvent *evt) {
	MEM_freeN(evt);
}

void wm_event_free_and_remove_from_queue_if_valid(wmEvent *evt) {
	LISTBASE_FOREACH(WindowManager *, wm, &G_MAIN->wm) {
		LISTBASE_FOREACH(wmWindow *, win, &wm->windows) {
			if (LIB_haslink(&win->event_queue, evt)) {
				LIB_remlink(&win->event_queue, evt);
				wm_event_free(evt);
				return;
			}
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

void WM_do_handlers(struct rContext *C) {
	WindowManager *wm = CTX_wm_manager(C);
	
	LISTBASE_FOREACH_MUTABLE(wmWindow *, window, &wm->windows) {
		wmEvent *evt;
		while (evt = (wmEvent *)window->event_queue.first) {
			CTX_wm_window_set(C, window);
			
			wm_window_make_drawable(wm, window);
			
			if(CTX_wm_window(C) == NULL) {
				wm_event_free_and_remove_from_queue_if_valid(evt);
				break;
			}
			
			LIB_remlink(&window->event_queue, evt);
			wm_event_update_last_handled(window, evt);
		}
		CTX_wm_window_set(C, NULL);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Event Translate
 * \{ */

ROSE_STATIC int convert_key(int tiny_window_key);
ROSE_STATIC int convert_btn(int tiny_window_btn);

ROSE_STATIC wmEvent *wm_event_add_mousemove(wmWindow *win, const wmEvent *evt) {
	wmEvent *event_last = (wmEvent *)win->event_queue.last;

	if(event_last && event_last->type == MOUSEMOVE) {
		event_last->type = INBETWEEN_MOUSEMOVE;
		event_last->flag = 0;
	}

	wmEvent *event_new = wm_event_add(win, evt);
	if(event_last == NULL) {
		event_last = win->event_state;
	}

	copy_v2_v2_int(event_new->prev_xy, event_last->mouse_xy);
	return event_new;
}

ROSE_STATIC void wm_event_prev_values_set(wmEvent *evt, wmEvent *event_state) {
	evt->prev_value = event_state->prev_value = event_state->value;
	evt->prev_value = event_state->prev_type = event_state->type;
}

ROSE_STATIC bool wm_event_is_double_click(wmEvent *evt, const double event_time, const double prev_press_event_time) {
	if((evt->type == evt->prev_type) && (evt->prev_value == KM_RELEASE) && (evt->value == KM_PRESS)) {
		if((event_time - prev_press_event_time) < 0.5) {
			return true;
		}
	}
	return false;
}

ROSE_STATIC void wm_event_prev_click_set(double event_time, wmEvent *event_state, double *prev_press_event_time) {
	event_state->prev_press_type = event_state->type;
	event_state->prev_press_modifier = event_state->modifier;
	copy_v2_v2_int(event_state->prev_press_xy, event_state->mouse_xy);
	*prev_press_event_time = event_time;
}

ROSE_STATIC void wm_event_state_update_and_click_set_ex(wmEvent *evt, double event_time, wmEvent *event_state, double *prev_press_event_time, bool is_keyboard, bool check_double_click) {
	ROSE_assert(ISKEYBOARD_OR_BUTTON(evt->type));
	ROSE_assert(ELEM(evt->value, KM_PRESS, KM_RELEASE));
	
	const int event_state_flag_mask = WM_EVENT_IS_REPEAT;

	wm_event_prev_values_set(evt, event_state);
	
	event_state->type = event_state->prev_type = evt->type;
	event_state->value = event_state->prev_value = evt->value;
	
	if(is_keyboard) {
		event_state->modifier = evt->modifier;
	}
	event_state->flag = evt->flag & event_state_flag_mask;
	
	if(check_double_click && wm_event_is_double_click(evt, event_time, *prev_press_event_time)) {
		evt->value = KM_DBL_CLICK;
	}
	else if(evt->value == KM_PRESS) {
		if((evt->flag & WM_EVENT_IS_REPEAT) == 0) {
			wm_event_prev_click_set(event_time, event_state, prev_press_event_time);
		}
	}
}
ROSE_STATIC void wm_event_state_update_and_click_set(wmEvent *evt, double event_time, wmEvent *event_state, double *prev_press_event_time, int tiny_window_type) {
	const bool is_keyboard = ELEM(tiny_window_type, WTK_EVT_KEYDOWN, WTK_EVT_KEYUP);
	
	wm_event_state_update_and_click_set_ex(evt, event_time, event_state, prev_press_event_time, is_keyboard, true);
}

ROSE_STATIC bool wm_event_is_same_key_press(const wmEvent *event_a, const wmEvent *event_b) {
	if (event_a->value != KM_PRESS || event_b->value != KM_PRESS) {
		return false;
	}
	if (event_a->modifier != event_b->modifier || event_a->type != event_b->type) {
		return false;
	}
	return true;
}

ROSE_STATIC bool wm_event_is_ignorable_key_press(const wmWindow *win, const wmEvent *evt) {
	if(LIB_listbase_is_empty(&win->event_queue)) {
		return false;
	}
	if((evt->flag & WM_EVENT_IS_REPEAT) == 0) {
		return false;
	}
	return wm_event_is_same_key_press((wmEvent *)win->event_queue.last, evt);
}

void wm_event_add_tiny_window_mouse_button(WindowManager *wm, wmWindow *window, int type, int button, int x, int y, double time) {
	ROSE_assert(ELEM(type, WTK_EVT_MOUSEMOVE, WTK_EVT_BUTTONDOWN, WTK_EVT_BUTTONUP));
	
	wmEvent nevt, *event_state = window->event_state, *evt = &nevt;
	memcpy(evt, event_state, sizeof(wmEvent));
	evt->flag = 0;
	
	evt->prev_type = evt->type;
	evt->prev_value = evt->value;
	
	if((event_state->type || event_state->value) && !(ISKEYBOARD_OR_BUTTON(event_state->type) || (event_state->type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}
	if((event_state->prev_type || event_state->prev_value) && !(ISKEYBOARD_OR_BUTTON(event_state->prev_type) || (event_state->prev_type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}
	
	evt->mouse_xy[0] = x;
	evt->mouse_xy[1] = y;
	
	switch (type) {
		case WTK_EVT_MOUSEMOVE: {
			evt->type = MOUSEMOVE;
			evt->value = KM_NOTHING;
			{
				wmEvent *event_new = wm_event_add_mousemove(window, evt);
				copy_v2_v2_int(event_state->mouse_xy, event_new->mouse_xy);
				copy_v2_v2_int(event_state->prev_xy, event_new->mouse_xy);
			}
		} break;
		case WTK_EVT_BUTTONDOWN:
		case WTK_EVT_BUTTONUP: {
			evt->type = convert_btn(button);
			if (evt->type == EVENT_NONE) {
				break;
			}

			if (type == WTK_EVT_BUTTONDOWN) {
				evt->value = KM_PRESS;
			}
			else {
				evt->value = KM_RELEASE;
			}
			
			wm_event_state_update_and_click_set(evt, time, event_state, &window->prev_press_event_time, type);
			copy_v2_v2_int(event_state->mouse_xy, evt->mouse_xy);
			copy_v2_v2_int(event_state->prev_xy, evt->mouse_xy);
			wm_event_add(window, evt);
		} break;
	}
}

void wm_event_add_tiny_window_key(WindowManager *wm, wmWindow *window, int type, int key, bool repeat, char utf8[4], double event_time) {
	ROSE_assert(ELEM(type, WTK_EVT_KEYDOWN, WTK_EVT_KEYUP));
	
	wmEvent nevt, *event_state = window->event_state, *evt = &nevt;
	memcpy(evt, event_state, sizeof(wmEvent));
	evt->flag = 0;
	
	evt->prev_type = evt->type;
	evt->prev_value = evt->value;
	
	if((event_state->type || event_state->value) && !(ISKEYBOARD_OR_BUTTON(event_state->type) || (event_state->type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}
	if((event_state->prev_type || event_state->prev_value) && !(ISKEYBOARD_OR_BUTTON(event_state->prev_type) || (event_state->prev_type == EVENT_NONE))) {
		/** Event state should only keep information about keyboard or buttons. */
		ROSE_assert_unreachable();
	}

	evt->mouse_xy[0] = event_state->mouse_xy[0];
	evt->mouse_xy[1] = event_state->mouse_xy[1];
	
	switch(type) {
		case WTK_EVT_KEYDOWN:
		case WTK_EVT_KEYUP: {
			evt->type = convert_key(key);
			if(evt->type == EVENT_NONE) {
				break;
			}
			SET_FLAG_FROM_TEST(evt->flag, repeat, WM_EVENT_IS_REPEAT);
			
			if(type == WTK_EVT_KEYDOWN) {
				memcpy(evt->input, utf8, sizeof(char[4]));
				evt->value = KM_PRESS;
			}
			else {
				memset(evt->input, 0, sizeof(char[4]));
				evt->value = KM_RELEASE;
			}
			
			switch(evt->type) {
				case EVT_LEFTSHIFTKEY:
				case EVT_RIGHTSHIFTKEY: {
					SET_FLAG_FROM_TEST(evt->modifier, (evt->value == KM_PRESS), KM_SHIFT);
				} break;
				case EVT_LEFTCTRLKEY:
				case EVT_RIGHTCTRLKEY: {
					SET_FLAG_FROM_TEST(evt->modifier, (evt->value == KM_PRESS), KM_CTRL);
				} break;
				case EVT_LEFTALTKEY:
				case EVT_RIGHTALTKEY: {
					SET_FLAG_FROM_TEST(evt->modifier, (evt->value == KM_PRESS), KM_ALT);
				} break;
				case EVT_OSKEY: {
					SET_FLAG_FROM_TEST(evt->modifier, (evt->value == KM_PRESS), KM_OSKEY);
				} break;
			}
			
			wm_event_state_update_and_click_set(evt, event_time, event_state, &window->prev_press_event_time, type);
			
			if (!wm_event_is_ignorable_key_press(window, evt)) {
				wm_event_add(window, evt);
			}
		} break;
	}
}

ROSE_STATIC int convert_key(int tiny_window_key) {
	if (tiny_window_key >= WTK_KEY_A && tiny_window_key <= WTK_KEY_Z) {
		return (EVT_AKEY + (tiny_window_key - WTK_KEY_A));
	}
	if (tiny_window_key >= WTK_KEY_0 && tiny_window_key <= WTK_KEY_9) {
		return (EVT_ZEROKEY + (tiny_window_key - WTK_KEY_0));
	}
	if (tiny_window_key >= WTK_KEY_NUMPAD_0 && tiny_window_key <= WTK_KEY_NUMPAD_9) {
		return (EVT_PAD0 + (tiny_window_key - WTK_KEY_NUMPAD_0));
	}
	if (tiny_window_key >= WTK_KEY_F1 && tiny_window_key <= WTK_KEY_F24) {
		return (EVT_F1KEY + (tiny_window_key - WTK_KEY_F1));
	}

#define ASSOCIATE(what, to) case what: return to;

	switch(tiny_window_key) {
		ASSOCIATE(WTK_KEY_BACKSPACE, EVT_BACKSPACEKEY);
		ASSOCIATE(WTK_KEY_TAB, EVT_TABKEY);
		ASSOCIATE(WTK_KEY_CLEAR, EVENT_NONE);
		ASSOCIATE(WTK_KEY_ENTER, EVT_RETKEY);
		ASSOCIATE(WTK_KEY_ESC, EVT_ESCKEY);
		ASSOCIATE(WTK_KEY_SPACE, EVT_SPACEKEY);
		ASSOCIATE(WTK_KEY_QUOTE, EVT_QUOTEKEY);
		ASSOCIATE(WTK_KEY_COMMA, EVT_COMMAKEY);
		ASSOCIATE(WTK_KEY_MINUS, EVT_MINUSKEY);
		ASSOCIATE(WTK_KEY_PLUS, EVT_PLUSKEY);
		ASSOCIATE(WTK_KEY_PERIOD, EVT_PERIODKEY);
		ASSOCIATE(WTK_KEY_SLASH, EVT_SLASHKEY);
		ASSOCIATE(WTK_KEY_SEMICOLON, EVT_SEMICOLONKEY);
		ASSOCIATE(WTK_KEY_EQUAL, EVT_EQUALKEY);
		ASSOCIATE(WTK_KEY_LEFT_BRACKET, EVT_LEFTBRACKETKEY);
		ASSOCIATE(WTK_KEY_RIGHT_BRACKET, EVT_RIGHTBRACKETKEY);
		ASSOCIATE(WTK_KEY_BACKSLASH, EVT_BACKSLASHKEY);
		ASSOCIATE(WTK_KEY_ACCENTGRAVE, EVT_ACCENTGRAVEKEY);
		ASSOCIATE(WTK_KEY_LEFT_SHIFT, EVT_LEFTSHIFTKEY);
		ASSOCIATE(WTK_KEY_RIGHT_SHIFT, EVT_RIGHTSHIFTKEY);
		ASSOCIATE(WTK_KEY_LEFT_CTRL, EVT_LEFTCTRLKEY);
		ASSOCIATE(WTK_KEY_RIGHT_CTRL, EVT_RIGHTCTRLKEY);
		ASSOCIATE(WTK_KEY_LEFT_OS, EVT_OSKEY);
		ASSOCIATE(WTK_KEY_RIGHT_OS, EVT_OSKEY);
		ASSOCIATE(WTK_KEY_LEFT_ALT, EVT_LEFTALTKEY);
		ASSOCIATE(WTK_KEY_RIGHT_ALT, EVT_RIGHTALTKEY);
		ASSOCIATE(WTK_KEY_APP, EVT_APPKEY);
		ASSOCIATE(WTK_KEY_CAPSLOCK, EVT_CAPSLOCKKEY);
		ASSOCIATE(WTK_KEY_NUMLOCK, EVENT_NONE);
		ASSOCIATE(WTK_KEY_SCRLOCK, EVENT_NONE);
		ASSOCIATE(WTK_KEY_LEFT, EVT_LEFTARROWKEY);
		ASSOCIATE(WTK_KEY_RIGHT, EVT_RIGHTARROWKEY);
		ASSOCIATE(WTK_KEY_UP, EVT_UPARROWKEY);
		ASSOCIATE(WTK_KEY_DOWN, EVT_DOWNARROWKEY);
		ASSOCIATE(WTK_KEY_PRTSCN, EVENT_NONE);
		ASSOCIATE(WTK_KEY_PAUSE, EVT_PAUSEKEY);
		ASSOCIATE(WTK_KEY_INSERT, EVT_INSERTKEY);
		ASSOCIATE(WTK_KEY_DELETE, EVT_DELKEY);
		ASSOCIATE(WTK_KEY_HOME, EVT_HOMEKEY);
		ASSOCIATE(WTK_KEY_END, EVT_ENDKEY);
		ASSOCIATE(WTK_KEY_PAGEUP, EVT_PAGEUPKEY);
		ASSOCIATE(WTK_KEY_PAGEDOWN, EVT_PAGEDOWNKEY);
		ASSOCIATE(WTK_KEY_NUMPAD_PERIOD, EVT_PADPERIOD);
		ASSOCIATE(WTK_KEY_NUMPAD_ENTER, EVT_PADENTER);
		ASSOCIATE(WTK_KEY_NUMPAD_PLUS, EVT_PADPLUSKEY);
		ASSOCIATE(WTK_KEY_NUMPAD_MINUS, EVT_PADMINUS);
		ASSOCIATE(WTK_KEY_NUMPAD_ASTERISK, EVT_PADASTERKEY);
		ASSOCIATE(WTK_KEY_NUMPAD_SLASH, EVT_PADSLASHKEY);
		ASSOCIATE(WTK_KEY_GRLESS, EVT_GRLESSKEY);
		ASSOCIATE(WTK_KEY_MEDIA_PLAY, EVT_MEDIAPLAY);
		ASSOCIATE(WTK_KEY_MEDIA_STOP, EVT_MEDIASTOP);
		ASSOCIATE(WTK_KEY_MEDIA_FIRST, EVT_MEDIAFIRST);
		ASSOCIATE(WTK_KEY_MEDIA_LAST, EVT_MEDIALAST);
	}

#undef ASSOCIATE

	return EVENT_NONE;
}

ROSE_STATIC int convert_btn(int tiny_window_btn) {
#define ASSOCIATE(what, to) case what: return to;

	switch (tiny_window_btn) {
		ASSOCIATE(WTK_BTN_LEFT, LEFTMOUSE);
		ASSOCIATE(WTK_BTN_MIDDLE, MIDDLEMOUSE);
		ASSOCIATE(WTK_BTN_RIGHT, RIGHTMOUSE);
	}

#undef ASSOCIATE

	return EVENT_NONE;
}

/** \} */
