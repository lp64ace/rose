#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#include "UI_interface.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "LIB_listbase.h"
#include "LIB_ghash.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "interface_intern.h"

#include "RFT_api.h"

#include <ctype.h>
#include <stdio.h>
#include <limits.h>

/* -------------------------------------------------------------------- */
/** \name UI Event Handlers
 * \{ */

ROSE_STATIC int ui_region_handler(struct rContext *C, const wmEvent *evt, void *user_data);

typedef struct uiHandleButtonData {
	wmWindow *window;
	ScrArea *area;
	ARegion *region;
	
	int state;
	
	int selini;
} uiHandleButtonData;

/** #uiHandleButtonData->state */
enum {
	BUTTON_STATE_TEXT_EDITING = 1,
	BUTTON_STATE_TEXT_SELECTING,
	BUTTON_STATE_WAIT_RELEASE,
};

bool ui_but_is_editing(uiBut *but) {
	uiHandleButtonData *data = but->active;
	return (data && data->state == BUTTON_STATE_TEXT_EDITING);
}

ROSE_INLINE bool button_modal_state(int state) {
	return ELEM(state, BUTTON_STATE_TEXT_EDITING, BUTTON_STATE_TEXT_SELECTING, BUTTON_STATE_WAIT_RELEASE);
}

ROSE_STATIC void ui_textedit_set_cursor_pos(uiBut *but, const ARegion *region, const float x) {
	float startx = x, starty = 0.0f;
	ui_window_to_block_fl(region, but->block, &startx, &starty);
	
	startx -= UI_TEXT_MARGIN_X;
	startx -= but->rect.xmin;
	
	int font = RFT_set_default();
	
	but->offset = (int)RFT_str_offset_from_cursor_position(font, but->name, -1, startx);
}

ROSE_STATIC int ui_handler_region_menu(struct rContext *C, const wmEvent *evt, void *user_data);

ROSE_STATIC void button_activate_state(struct rContext *C, uiBut *but, int state) {
	uiHandleButtonData *data = but->active;
	if (data->state == state) {
		return;
	}

	// prev was modal, new is not modal!
	if(button_modal_state(data->state) && !button_modal_state(state)) {
		WM_event_remove_ui_handler(&data->window->modalhandlers, ui_handler_region_menu, NULL, data, true);
	}

	// prev was not modal, new is modal!
	if(button_modal_state(state) && !button_modal_state(data->state)) {
		WM_event_add_ui_handler(C, &data->window->modalhandlers, ui_handler_region_menu, NULL, data, 0);
	}

	data->state = state;
}

void ui_do_but_activate_init(struct rContext *C, ARegion *region, uiBut *but) {
	/* Only ever one active button! */
	ROSE_assert(ui_region_find_active_but(region) == NULL);

	uiHandleButtonData *data = MEM_callocN(sizeof(uiHandleButtonData), "uiHandleButtonData");

	data->window = CTX_wm_window(C);
	data->area = CTX_wm_area(C);
	ROSE_assert(region != NULL);
	data->region = region;

	/**
	 * Activate button.
	 * Sets the hover flag to enable button highlights,
	 * usually the button is initially activated because it's hovered.
	 */
	but->flag |= UI_HOVER;
	but->active = data;

	switch (but->type) {
		case UI_BTYPE_EDIT: {
			/** Initially we start as text selecting if the user decides to click then we start text-editing. */
			button_activate_state(C, but, BUTTON_STATE_TEXT_SELECTING);
			data->selini = -1;
		} break;
		default: {
			/** We have marked this button as hovered and we await release of the flag. */
			button_activate_state(C, but, BUTTON_STATE_WAIT_RELEASE);
		} break;
	}
}

void ui_do_but_activate_exit(struct rContext *C, ARegion *region, uiBut *but) {
	if (!but || !but->active) {
		return;
	}
	
	uiHandleButtonData *data = but->active;
	
	if (button_modal_state(data->state)) {
		WM_event_remove_ui_handler(&data->window->modalhandlers, ui_handler_region_menu, NULL, data, true);
	}

	but->flag &= ~(UI_HOVER | UI_SELECT);
	but->selsta = but->selend = but->offset = 0;

	MEM_SAFE_FREE(but->active);
}

void ui_but_active_free(struct rContext *C, uiBut *but) {
	if (but->active) {
		uiHandleButtonData *data = but->active;
		ui_do_but_activate_exit(C, data->region, but);
	}
}

ROSE_STATIC void ui_but_activate(struct rContext *C, ARegion *region, uiBut *but) {
	uiBut *old = ui_region_find_active_but(region);
	if (old) {
		uiHandleButtonData *data = old->active;

		ui_do_but_activate_exit(C, region, old);
	}
	ui_do_but_activate_init(C, region, but);
}

enum eTextEditDelete {
	TE_DELETE_PREV,
	TE_DELETE_NEXT,
	TE_DELETE_SELECT,
};

ROSE_STATIC bool ui_textedit_delete_selection(uiBut *but) {
	unsigned int length = (unsigned int)LIB_strlen(but->name);
	
	bool changed = false;

	if(but->selsta != but->selend && length) {
		memmove(but->name + but->selsta, but->name + but->selend, length - but->selend + 1);
		changed |= true;
	}
	but->offset = but->selend = but->selsta;

	return changed;
}

ROSE_STATIC void ui_textedit_cursor_select(uiBut *but, uiHandleButtonData *data, const float x) {
	ui_textedit_set_cursor_pos(but, data->region, x);
	
	but->selsta = but->offset;
	but->selend = (data->selini >= 0) ? data->selini : but->offset;
	if(but->selend < but->selsta) {
		SWAP(int, but->selsta, but->selend);
	}
}

ROSE_STATIC void ui_textedit_move(uiBut *but, int direction, bool jump, bool select) {
	unsigned int length = (unsigned int)LIB_strlen(but->name);

	if ((but->selend - but->selsta) > 0 && !select) {
		if (jump) {
			// If we are to jump everything to that direction go the end or to the start.
			but->selsta = but->selend = but->offset = (direction > 0) ? length : 0;
		}
		else if (direction > 0) {
			but->selsta = but->offset = but->selend; // End of selection
		}
		else if (direction < 0) {
			but->selend = but->offset = but->selsta;  // Start of selection
		}
	}
	else {
		const int prev = but->offset;
		if (jump) {
			but->offset = (direction > 0) ? length : 0;
		}
		else {
			for (int i = 0; i < direction; i++) {
				LIB_str_cursor_step_next_utf8(but->name, length, &but->offset);
			}
			for (int i = 0; i > direction; i--) {
				LIB_str_cursor_step_prev_utf8(but->name, length, &but->offset);
			}
		}
		if (select) {
			if ((but->selend - but->selsta) == 0) {
				but->selsta = but->offset;
				but->selend = prev;
			}
			else if (but->selsta == prev) {
				but->selsta = but->offset;
			}
			else if (but->selend == prev) {
				but->selend = but->offset;
			}
		}
	}

	if (but->selend < but->selsta) {
		SWAP(int, but->selsta, but->selend);
	}
}

ROSE_STATIC bool ui_textedit_delete(uiBut *but, const wmEvent *evt) {
	unsigned int length = (unsigned int)LIB_strlen(but->name);

	bool changed = false;

	switch (evt->type) {
		case EVT_BACKSPACEKEY: {
			if ((but->selend - but->selsta) > 0) {
				changed |= ui_textedit_delete_selection(but);
			}
			else if (but->offset > 0 && but->offset <= length) {
				int pos = but->offset;
				LIB_str_cursor_step_prev_utf8(but->name, length, &pos);
				memmove(but->name + pos, but->name + but->offset, length - pos + 1);
				but->offset = pos;
				changed |= true;
			}
		} break;
		case EVT_DELKEY: {
			if ((but->selend - but->selsta) > 0) {
				changed |= ui_textedit_delete_selection(but);
			}
			else if (but->offset >= 0 && but->offset < length) {
				int pos = but->offset;
				LIB_str_cursor_step_next_utf8(but->name, length, &pos);
				memmove(but->name + but->offset, but->name + pos, length - pos + 1);
				changed |= true;
			}
		} break;
	}

	return changed;
}

ROSE_STATIC bool ui_textedit_insert_buf(uiBut *but, const char *input, unsigned int step) {
	unsigned int length = (unsigned int)LIB_strlen(but->name);
	unsigned int cpy = length - (but->selend - but->selsta) + 1;
	
	bool changed = false;
	
	const unsigned int maxlength = 64;
	if (cpy < maxlength) {
		if ((but->selend - but->selsta) > 0) {
			ui_textedit_delete_selection(but);
			length = (unsigned int)LIB_strlen(but->name);
			changed |= true;
		}
		
		if(step && (length + step < maxlength)) {
			char *nbuf = LIB_strformat_allocN("%.*s%.*s%s", but->offset, but->name, step, input, but->name + but->offset);
			MEM_freeN(but->name);
			but->name = nbuf;
			
			but->offset += step;
			changed |= true;
		}
	}
	
	return changed;
}

enum {
	UI_TEXTEDIT_PASTE = 1,
	UI_TEXTEDIT_COPY,
	UI_TEXTEDIT_CUT,
};

ROSE_STATIC bool ui_textedit_copypaste(struct rContext *C, uiBut *but, const int mode) {
	bool changed = false;

	unsigned int length = 0;

	switch (mode) {
		case UI_TEXTEDIT_PASTE: {
			char *nbuf = WM_clipboard_text_get_firstline(C, false, &length);

			if (nbuf) {
				changed |= ui_textedit_insert_buf(but, nbuf, length);
				MEM_freeN(nbuf);
			}
		} break;
		case UI_TEXTEDIT_COPY:
		case UI_TEXTEDIT_CUT: {
			if (but->selend - but->selsta > 0) {
				char *nbuf = LIB_strndupN(but->name + but->selsta, but->selend - but->selsta);
				WM_clipboard_text_set(C, nbuf, false);
				MEM_freeN(nbuf);

				if (mode == UI_TEXTEDIT_CUT) {
					changed |= ui_textedit_delete_selection(but);
				}
			}
		} break;
	}

	return changed;
}

ROSE_STATIC int ui_do_but_textsel(struct rContext *C, uiBlock *block, uiBut *but, uiHandleButtonData *data, const wmEvent *evt) {
	switch (evt->type) {
		case MOUSEMOVE: {
			ui_textedit_cursor_select(but, data, evt->mouse_xy[0]);
			
			if (data->selini < 0) {
				if (!ui_but_contains_px(but, data->region, evt->mouse_xy)) {
					ui_do_but_activate_exit(C, data->region, but);
				}
			}
		} break;
		case LEFTMOUSE: {
			ui_textedit_cursor_select(but, data, evt->mouse_xy[0]);
			if (evt->value == KM_RELEASE) {
				button_activate_state(C, but, BUTTON_STATE_TEXT_EDITING);
			}
			else {
				data->selini = but->offset;
			}
			return WM_UI_HANDLER_BREAK;
		} break;
	}

	return WM_UI_HANDLER_CONTINUE;
}

ROSE_STATIC int ui_do_but_textedit(struct rContext *C, uiBlock *block, uiBut *but, uiHandleButtonData *data, const wmEvent *evt) {
	if (!ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
		return WM_UI_HANDLER_CONTINUE;
	}

	int retval = WM_UI_HANDLER_CONTINUE;

	bool changed = false;

	switch (evt->type) {
		case LEFTMOUSE: {
			bool selection = (but->selend - but->selsta) > 0;
			
			bool is_press_in_button = false;
			if (ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
				if (ui_but_contains_px(but, data->region, evt->mouse_xy)) {
					is_press_in_button = true;
				}
			}

			if (ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
				if (is_press_in_button) {
					ui_textedit_set_cursor_pos(but, data->region, evt->mouse_xy[0]);
					but->selsta = but->selend = but->offset;
					data->selini = but->offset;

					if (evt->value == KM_PRESS) {
						button_activate_state(C, but, BUTTON_STATE_TEXT_SELECTING);
					}
					retval |= WM_UI_HANDLER_BREAK;
				}
				else {
					ui_do_but_activate_exit(C, data->region, but);
				}
			}

			if (evt->value == KM_DBL_CLICK && !selection) {
				if (is_press_in_button) {
					unsigned int length = LIB_strlen(but->name);
					LIB_str_cursor_step_bounds_utf8(but->name, length, but->offset, &but->selsta, &but->selend);
					but->offset = but->selend;
				}
			}
		} break;
		case EVT_ESCKEY: {
			ui_do_but_activate_exit(C, data->region, but);
		} break;
		case EVT_BACKSPACEKEY:
		case EVT_DELKEY: {
			changed |= ui_textedit_delete(but, evt);
		} break;
		case EVT_RIGHTARROWKEY:
		case EVT_LEFTARROWKEY: {
			int direction = (evt->type == EVT_LEFTARROWKEY) ? -1 : 1;

			ui_textedit_move(but, direction, false, evt->modifier & KM_SHIFT);

			retval |= WM_UI_HANDLER_BREAK;
		} break;
		case EVT_HOMEKEY:
		case EVT_ENDKEY: {
			int direction = (evt->type == EVT_HOMEKEY) ? -1 : 1;

			ui_textedit_move(but, direction, true, evt->modifier & KM_SHIFT);

			retval |= WM_UI_HANDLER_BREAK;
		} break;
	}

	if (evt->value == KM_PRESS) {
		switch (evt->type) {
			case EVT_CKEY:
			case EVT_VKEY:
			case EVT_XKEY: {
#if defined(__APPLE__)
				if (ELEM(evt->modifier, KM_OSKEY, KM_CTRL)) {
#else
				if (ELEM(evt->modifier, KM_CTRL)) {
#endif
					if (evt->type == EVT_CKEY) {
						changed |= ui_textedit_copypaste(C, but, UI_TEXTEDIT_COPY);
					}
					if (evt->type == EVT_VKEY) {
						changed |= ui_textedit_copypaste(C, but, UI_TEXTEDIT_PASTE);
					}
					if (evt->type == EVT_XKEY) {
						changed |= ui_textedit_copypaste(C, but, UI_TEXTEDIT_CUT);
					}
				}
			} break;
			case EVT_AKEY: {
#if defined(__APPLE__)
				if (ELEM(evt->modifier, KM_OSKEY, KM_CTRL)) {
#else
				if (ELEM(evt->modifier, KM_CTRL)) {
#endif
					unsigned int length = LIB_strlen(but->name);

					but->selsta = 0;
					but->selend = length;
					but->offset = but->selend;
				}
			} break;
		}
	}

	if (evt->input[0] && retval == WM_UI_HANDLER_CONTINUE) {
		changed |= ui_textedit_insert_buf(but, evt->input, LIB_str_utf8_size_or_error(evt->input));
	}

	if (changed) {
		retval |= WM_UI_HANDLER_BREAK;
	}

	return retval;
}

ROSE_STATIC int ui_handle_button_event(struct rContext *C, const wmEvent *evt, uiBut *but) {
	int retval = WM_UI_HANDLER_CONTINUE;

	uiHandleButtonData *data = but->active;
	
	switch (data->state) {
		case BUTTON_STATE_TEXT_EDITING: {
			retval |= ui_do_but_textedit(C, but->block, but, but->active, evt);
		} break;
		case BUTTON_STATE_TEXT_SELECTING: {
			retval |= ui_do_but_textsel(C, but->block, but, but->active, evt);
		} break;
		case BUTTON_STATE_WAIT_RELEASE: {
			if (!ui_but_contains_px(but, data->region, evt->mouse_xy)) {
				ui_do_but_activate_exit(C, data->region, but);
			}
		} break;
	}

	if (retval != WM_UI_HANDLER_CONTINUE) {
		return retval;
	}

	switch (evt->type) {
		case WINDEACTIVATE: {
			ui_do_but_activate_exit(C, data->region, but);
		} break;
	}

	return retval;
}

ROSE_STATIC int ui_handle_button_over(struct rContext *C, const wmEvent *evt, ARegion *region) {
	uiBut *but = ui_but_find_mouse_over_ex(region, evt->mouse_xy);
	if (but) {
		ui_but_activate(C, region, but);
		ui_handle_button_event(C, evt, but);
	}
	
	return WM_UI_HANDLER_CONTINUE;
}

ROSE_STATIC int ui_handler_region_menu(struct rContext *C, const wmEvent *evt, void *user_data) {
	uiHandleButtonData *data = (uiHandleButtonData *)user_data;
	
	if(data) {
		uiBut *but = ui_region_find_active_but(data->region);
		
		ui_handle_button_event(C, evt, but);
	}
	
	/** This is a modal handler don't allow anything to interfere with us! */
	return WM_UI_HANDLER_BREAK;
}

ROSE_STATIC int ui_region_handler(struct rContext *C, const wmEvent *evt, void *user_data) {
	ARegion *region = CTX_wm_region(C);
	int retval = WM_UI_HANDLER_CONTINUE;

	if (region == NULL || LIB_listbase_is_empty(&region->uiblocks)) {
		return retval;
	}
	
	uiBut *but = ui_region_find_active_but(region);
	
	if (but) {
		retval |= ui_handle_button_event(C, evt, but);
	}
	else {
		retval |= ui_handle_button_over(C, evt, region);
	}
	
	return retval;
}

ROSE_STATIC void ui_region_handler_remove(struct rContext *C, void *user_data) {
}

void UI_region_handlers_add(ListBase *handlers) {
	/** We just move the handler to another listbase, we don't need to call #ui_region_handler_remove. */
	WM_event_remove_ui_handler(handlers, ui_region_handler, ui_region_handler_remove, NULL, false);
	WM_event_add_ui_handler(NULL, handlers, ui_region_handler, ui_region_handler_remove, NULL, 0);
}

/** \} */
