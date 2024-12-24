#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"

#include "UI_interface.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_string_utf.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "interface_intern.h"

#include "RFT_api.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* -------------------------------------------------------------------- */
/** \name UI Event Handlers
 * \{ */

ROSE_STATIC int ui_region_handler(struct rContext *C, const wmEvent *evt, void *user_data);

typedef struct uiHandleButtonData {
	struct wmWindow *window;
	struct ScrArea *area;
	struct ARegion *region;

	int state;

	int selini;

	struct uiUndoStack_Text *undo_stack;
	struct uiPopupBlockHandle *menu;
} uiHandleButtonData;

/** #uiHandleButtonData->state */
enum {
#define _BUTTON_STATE_BEGIN BUTTON_STATE_HIGHLIGHT
	BUTTON_STATE_HIGHLIGHT = 1,
	BUTTON_STATE_TEXT_EDITING,
	BUTTON_STATE_TEXT_SELECTING,
	BUTTON_STATE_WAIT_RELEASE,
	BUTTON_STATE_MENU_OPEN,
	BUTTON_STATE_EXIT,
#define _BUTTON_STATE_END BUTTON_STATE_EXIT
};

bool ui_but_is_editing(uiBut *but) {
	uiHandleButtonData *data = but->active;
	return (data && data->state == BUTTON_STATE_TEXT_EDITING);
}

ROSE_INLINE bool button_modal_state(int state) {
	return IN_RANGE_INCL(state, _BUTTON_STATE_BEGIN, _BUTTON_STATE_END);
}

ROSE_STATIC int ui_handler_region_menu(struct rContext *C, const wmEvent *evt, void *user_data);

void ui_do_but_activate_init(struct rContext *C, ARegion *region, uiBut *but, int state);
void ui_do_but_activate_exit(struct rContext *C, ARegion *region, uiBut *but);

ROSE_STATIC void ui_but_tag_redraw(uiBut *but) {
	uiHandleButtonData *data = but->active;
	if (!data) {
		return;
	}

	ED_region_tag_redraw_no_rebuild(data->region);
}

ROSE_STATIC void button_activate_state(struct rContext *C, uiBut *but, int state) {
	uiHandleButtonData *data = but->active;
	if (!data || data->state == state) {
		return;
	}

	// prev was modal, new is not modal!
	if (button_modal_state(data->state) && !button_modal_state(state)) {
		WM_event_remove_ui_handler(&data->window->modalhandlers, ui_handler_region_menu, NULL, data, true);
	}

	// prev was not modal, new is modal!
	if (button_modal_state(state) && !button_modal_state(data->state)) {
		WM_event_add_ui_handler(C, &data->window->modalhandlers, ui_handler_region_menu, NULL, data, 0);
	}

	/** When highlighted the button is just hovered otherwise mark it selected! */
	SET_FLAG_FROM_TEST(but->flag, state != BUTTON_STATE_HIGHLIGHT, UI_SELECT);

	if (state == BUTTON_STATE_MENU_OPEN) {
		data->menu = ui_popup_block_create(C, data->region, but, but->menu_create_func, NULL, but->arg);
	}
	else if (data->menu) {
		ui_popup_block_free(C, data->menu);
		data->menu = NULL;
	}

	data->state = state;

	ED_region_tag_redraw_no_rebuild(data->region);

	if (data->state == BUTTON_STATE_EXIT) {
		ui_do_but_activate_exit(C, data->region, but);
	}
}

void ui_do_but_activate_init(struct rContext *C, ARegion *region, uiBut *but, int state) {
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
	
	switch (state) {
		case BUTTON_STATE_TEXT_SELECTING: {
			data->selini = -1;
		} break;
	}

	button_activate_state(C, but, state);

	switch (but->type) {
		case UI_BTYPE_EDIT: {
			data->undo_stack = ui_textedit_undo_stack_create();
			if (state == BUTTON_STATE_TEXT_EDITING) {
				but->selsta = 0;
				but->selend = but->offset = LIB_strlen(but->drawstr);
			}
			ui_textedit_undo_push(data->undo_stack, but->drawstr, LIB_strlen(but->drawstr));
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

	if (data->undo_stack) {
		ui_textedit_undo_stack_destroy(data->undo_stack);
	}

	ROSE_assert(!data->menu);

	MEM_SAFE_FREE(but->active);
}

void ui_but_active_free(struct rContext *C, uiBut *but) {
	if (but->active) {
		uiHandleButtonData *data = but->active;
		button_activate_state(C, but, BUTTON_STATE_EXIT);
	}
}

ROSE_STATIC void ui_but_activate(struct rContext *C, ARegion *region, uiBut *but, int state) {
	uiBut *old = ui_region_find_active_but(region);
	if (old) {
		uiHandleButtonData *data = old->active;
		button_activate_state(C, old, BUTTON_STATE_EXIT);
	}
	ui_do_but_activate_init(C, region, but, state);
}

ROSE_STATIC void ui_do_but_menu_exit(struct rContext *C, uiBut *but) {
	if (but->block && but->block->handle) {
		uiPopupBlockHandle *handle = but->block->handle;

		if (handle->popup_create_vars.but) {
			ui_do_but_menu_exit(C, handle->popup_create_vars.but);
		}
		else {
			ui_popup_block_free(C, handle);
		}
	}
	else {
		button_activate_state(C, but, BUTTON_STATE_EXIT);
	}
}

ROSE_STATIC void ui_apply_but_func(struct rContext *C, uiBut *but, void *arg1, void *arg2) {
	if (but->handle_func) {
		but->handle_func(C, but, arg1, arg2);
	}

	/** We are currently inside a popup menu and a button was handled, we need to exit the menu! */
	if (but->block && but->block->handle) {
		ui_do_but_menu_exit(C, but);
	}
}

ROSE_STATIC void ui_textedit_set_cursor_pos(uiBut *but, const ARegion *region, const float x) {
	int font = RFT_set_default();

	float startx = but->rect.xmin + UI_TEXT_MARGIN_X, starty = 0.0f;
	if ((but->draw & UI_BUT_TEXT_LEFT) == 0) {
		float width = ROSE_MIN(LIB_rctf_size_x(&but->rect) - 2 * UI_TEXT_MARGIN_X, RFT_width(font, but->drawstr + but->scroll, -1));
		startx = (but->draw & UI_BUT_TEXT_RIGHT) ? startx = but->rect.xmax - UI_TEXT_MARGIN_X - width : LIB_rctf_cent_x(&but->rect) - width / 2;
	}
	ui_block_to_window_fl(region, but->block, &startx, &starty);

	const int old = but->offset;

	/** mouse dragged outside the widget to the left. */
	if (x < startx) {
		int i = but->scroll;

		const char *last = &but->drawstr[but->scroll];

		while (i > 0) {
			if (LIB_str_cursor_step_prev_utf8(but->drawstr, but->scroll, &i)) {
				if (RFT_width(ui_but_text_font(but), but->drawstr + but->scroll, (last - but->drawstr) - i) > (startx - x) * 0.25f) {
					break;
				}
			}
			else {
				break;
			}
		}

		but->scroll = i;
		but->offset = i;
	}
	else {
		but->offset = but->scroll + RFT_str_offset_from_cursor_position(font, but->drawstr + but->scroll, INT_MAX, x - startx);
	}

	if (but->offset != old) {
		ui_but_tag_redraw(but);
	}
}

enum eTextEditDelete {
	TE_DELETE_PREV,
	TE_DELETE_NEXT,
	TE_DELETE_SELECT,
};

ROSE_STATIC bool ui_textedit_delete_selection(uiBut *but) {
	unsigned int length = (unsigned int)LIB_strlen(but->drawstr);

	bool changed = false;

	if (but->selsta != but->selend && length) {
		memmove(but->drawstr + but->selsta, but->drawstr + but->selend, length - but->selend + 1);
		changed |= true;
	}
	but->offset = but->selend = but->selsta;

	return changed;
}

ROSE_STATIC void ui_textedit_cursor_select(uiBut *but, uiHandleButtonData *data, const float x) {
	ui_textedit_set_cursor_pos(but, data->region, x);

	but->selsta = but->offset;
	but->selend = (data->selini >= 0) ? data->selini : but->offset;
	if (but->selend < but->selsta) {
		SWAP(int, but->selsta, but->selend);
	}
}

ROSE_STATIC void ui_textedit_move(uiBut *but, int direction, bool jump, bool select) {
	unsigned int length = (unsigned int)LIB_strlen(but->drawstr);

	if ((but->selend - but->selsta) > 0 && !select) {
		if (jump) {
			// If we are to jump everything to that direction go the end or to the start.
			but->selsta = but->selend = but->offset = (direction > 0) ? length : 0;
		}
		else if (direction > 0) {
			but->selsta = but->offset = but->selend;  // End of selection
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
				LIB_str_cursor_step_next_utf8(but->drawstr, length, &but->offset);
			}
			for (int i = 0; i > direction; i--) {
				LIB_str_cursor_step_prev_utf8(but->drawstr, length, &but->offset);
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

	ui_but_tag_redraw(but);
}

ROSE_STATIC bool ui_textedit_delete(uiBut *but, const wmEvent *evt) {
	unsigned int length = (unsigned int)LIB_strlen(but->drawstr);

	bool changed = false;

	switch (evt->type) {
		case EVT_BACKSPACEKEY: {
			if ((but->selend - but->selsta) > 0) {
				changed |= ui_textedit_delete_selection(but);
			}
			else if (but->offset > 0 && but->offset <= length) {
				int pos = but->offset;
				LIB_str_cursor_step_prev_utf8(but->drawstr, length, &pos);
				memmove(but->drawstr + pos, but->drawstr + but->offset, length - pos + 1);
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
				LIB_str_cursor_step_next_utf8(but->drawstr, length, &pos);
				memmove(but->drawstr + but->offset, but->drawstr + pos, length - pos + 1);
				changed |= true;
			}
		} break;
	}

	return changed;
}

ROSE_STATIC bool ui_textedit_insert_buf(uiBut *but, const char *input, unsigned int step) {
	unsigned int length = (unsigned int)LIB_strlen(but->drawstr);
	unsigned int cpy = length - (but->selend - but->selsta) + 1;

	bool changed = false;

	const unsigned int maxlength = but->maxlength;
	if (cpy < maxlength) {
		if ((but->selend - but->selsta) > 0) {
			ui_textedit_delete_selection(but);
			length = (unsigned int)LIB_strlen(but->drawstr);
			changed |= true;
		}

		if (step && (length + step < maxlength)) {
			char *nbuf = LIB_strformat_allocN("%.*s%.*s%s", but->offset, but->drawstr, step, input, but->drawstr + but->offset);
			MEM_freeN(but->drawstr);
			but->drawstr = nbuf;

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
				char *nbuf = LIB_strndupN(but->drawstr + but->selsta, but->selend - but->selsta);
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

ROSE_STATIC bool ui_textedit_set(uiBut *but, const char *text) {
	if (text) {
		MEM_freeN(but->drawstr);
		but->drawstr = LIB_strdupN(text);
		but->selsta = but->selend = but->offset = LIB_strlen(but->drawstr);
		ui_but_tag_redraw(but);
		return true;
	}
	return false;
}

ROSE_STATIC int ui_do_but_textsel(struct rContext *C, uiBlock *block, uiBut *but, uiHandleButtonData *data, const wmEvent *evt) {
	int retval = WM_UI_HANDLER_CONTINUE;
	switch (evt->type) {
		case MOUSEMOVE: {
			ui_textedit_cursor_select(but, data, evt->mouse_xy[0]);
			if (data->selini < 0) {
				if (!ui_but_contains_px(but, data->region, evt->mouse_xy)) {
					/** This should cancel the editing and restore the original string. */
					button_activate_state(C, but, BUTTON_STATE_EXIT);
				}
			}
			retval |= WM_UI_HANDLER_BREAK;
		} break;
		case MOUSELEAVE: {
			button_activate_state(C, but, BUTTON_STATE_EXIT);
			retval |= WM_UI_HANDLER_BREAK;
		} break;
		case LEFTMOUSE: {
			ui_textedit_cursor_select(but, data, evt->mouse_xy[0]);
			if (evt->value == KM_RELEASE) {
				button_activate_state(C, but, BUTTON_STATE_TEXT_EDITING);
			}
			else {
				data->selini = but->offset;
			}
			retval |= WM_UI_HANDLER_BREAK;
		} break;
	}

	return retval;
}

ROSE_STATIC bool ui_but_is_editable_as_text(uiBut *but) {
	return ELEM(but->type, UI_BTYPE_EDIT);
}

ROSE_STATIC uiBut *ui_textedit_next_but(uiBlock *block, uiBut *but, uiHandleButtonData *data) {
	if(ELEM(but->type, UI_BTYPE_SEPR, UI_BTYPE_HSPR, UI_BTYPE_VSPR)) {
		return NULL;
	}
	
	for(uiBut *other = but->next; other; other = other->next) {
		if (ui_but_is_editable_as_text(other)) {
			return other;
		}
	}
	for(uiBut *other = (uiBut *)block->buttons.first; other; other = other->next) {
		if (ui_but_is_editable_as_text(other)) {
			return other;
		}
	}
	
	return NULL;
}

ROSE_STATIC uiBut *ui_textedit_prev_but(uiBlock *block, uiBut *but, uiHandleButtonData *data) {
	if(ELEM(but->type, UI_BTYPE_SEPR, UI_BTYPE_HSPR, UI_BTYPE_VSPR)) {
		return NULL;
	}
	
	for(uiBut *other = but->prev; other; other = other->prev) {
		if (ui_but_is_editable_as_text(other)) {
			return other;
		}
	}
	for(uiBut *other = (uiBut *)block->buttons.last; other; other = other->prev) {
		if (ui_but_is_editable_as_text(other)) {
			return other;
		}
	}
	
	return NULL;
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
					ui_but_update(but);
					button_activate_state(C, but, BUTTON_STATE_EXIT);
					ui_apply_but_func(C, but, but->arg1, but->arg2);
				}
			}

			if (evt->value == KM_DBL_CLICK && !selection) {
				if (is_press_in_button) {
					unsigned int length = LIB_strlen(but->drawstr);
					LIB_str_cursor_step_bounds_utf8(but->drawstr, length, but->offset, &but->selsta, &but->selend);
					but->offset = but->selend;
				}
			}
		} break;
		case EVT_RETKEY:
		case EVT_PADENTER: {
			ui_but_update(but);
			button_activate_state(C, but, BUTTON_STATE_EXIT);
			ui_apply_but_func(C, but, but->arg1, but->arg2);
		} break;
		case EVT_ESCKEY: {
			button_activate_state(C, but, BUTTON_STATE_EXIT);
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

	if (ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
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
					but->selsta = 0;
					but->selend = but->offset = LIB_strlen(but->drawstr);
					ui_but_tag_redraw(but);
				}
			} break;
			case EVT_ZKEY:
			case EVT_YKEY: {
				int direction = ((evt->modifier & KM_SHIFT) != 0) ? 1 : -1;
				int multiply = (evt->type == EVT_ZKEY) ? 1 : -1;

				if (
#if defined(__APPLE__)
					((evt->modifier & KM_OSKEY) != 0 && ((evt->modifier & (KM_ALT | KM_CTRL)) == 0)) ||
#endif
					((evt->modifier & KM_CTRL) != 0 && ((evt->modifier & (KM_ALT | KM_OSKEY)) == 0))) {
					int offset;

					const char *text = ui_textedit_undo(data->undo_stack, direction * multiply, &offset);
					if (ui_textedit_set(but, text)) {
						but->selsta = but->selend = but->offset = offset;
					}

					retval |= WM_UI_HANDLER_BREAK;
				}
			} break;
			case EVT_TABKEY: {
				if ((evt->modifier & (KM_CTRL | KM_ALT | KM_OSKEY)) == 0) {
					uiBut *other = NULL;
					if (evt->modifier & KM_SHIFT) {
						other = ui_textedit_prev_but(block, but, data);
					}
					else {
						other = ui_textedit_next_but(block, but, data);
					}
					
					if (other && other != but) {
						ui_but_activate(C, data->region, other, BUTTON_STATE_TEXT_EDITING);
						retval |= WM_UI_HANDLER_BREAK;
					}
				}
			} break;
		}
	}

	if (retval == WM_UI_HANDLER_CONTINUE) {
		if (evt->input[0]) {
			changed |= ui_textedit_insert_buf(but, evt->input, LIB_str_utf8_size_or_error(evt->input));
		}
	}

	if (changed) {
		ui_textedit_undo_push(data->undo_stack, but->drawstr, but->offset);
		ui_but_tag_redraw(but);

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
		case BUTTON_STATE_HIGHLIGHT: {
			if (!ui_but_contains_px(but, data->region, evt->mouse_xy)) {
				button_activate_state(C, but, BUTTON_STATE_EXIT);
				break;
			}
			if (evt->type == LEFTMOUSE && ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
				/** This type of buttons only activate when double-clicked! */
				if (!ELEM(but->type, UI_BTYPE_TEXT) || ELEM(evt->value, KM_DBL_CLICK)) {
					button_activate_state(C, but, BUTTON_STATE_WAIT_RELEASE);
					retval |= WM_UI_HANDLER_BREAK;
				}
			}
		} break;
		case BUTTON_STATE_WAIT_RELEASE: {
			if (evt->type == LEFTMOUSE && evt->value == KM_RELEASE) {
				if (!ui_but_contains_px(but, data->region, evt->mouse_xy)) {
					button_activate_state(C, but, BUTTON_STATE_EXIT);
					break;
				}
				/** Menu was triggered open the menu! */
				else if (but->type == UI_BTYPE_MENU) {
					button_activate_state(C, but, BUTTON_STATE_MENU_OPEN);
				}
				else {
					button_activate_state(C, but, BUTTON_STATE_HIGHLIGHT);
					ui_apply_but_func(C, but, but->arg1, but->arg2);
				}
				retval |= WM_UI_HANDLER_BREAK;
			}
		} break;
		case BUTTON_STATE_MENU_OPEN: {
			/** Right mouse anywhere cancels the popup. */
			if (evt->type == RIGHTMOUSE && ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
				button_activate_state(C, but, BUTTON_STATE_EXIT);
				retval |= WM_UI_HANDLER_BREAK;
				break;
			}
			if (ui_but_contains_px(but, data->region, evt->mouse_xy)) {
				retval |= WM_UI_HANDLER_BREAK;
				break;
			}
			/** Left mouse outside of the region cancels the popup. */
			if (evt->type == LEFTMOUSE && ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
				ARegion *menuregion = NULL;
				if (data->menu && data->menu->region) {
					if (ED_region_contains_xy(data->menu->region, evt->mouse_xy)) {
						retval |= WM_UI_HANDLER_BREAK;
						break;
					}
					uiBut *sub = ui_block_active_but_get((uiBlock *)data->menu->region->uiblocks.first);
					if (sub) {
						retval |= WM_UI_HANDLER_BREAK;
						break;
					}
					menuregion = data->region;
				}
				button_activate_state(C, but, BUTTON_STATE_EXIT);
				/**
				 * A popup was closed because the current popup receive a left-mouse click event, 
				 * even though the popup was not able to handle it parent pop-ups may be able to instead!
				 * 
				 * The parent region has priority, send the event to the active in there, 
				 * if that doesn't find a hovered-button either the chain keeps going up, 
				 * until eventually all the popup regions close or one of them is triggered!
				 * 
				 * NOTE: We do not update the active button since it may no longer has the focus, 
				 * we try to find the hovered one instead!
				 */
				retval |= ui_handle_button_over(C, evt, menuregion);
			}
		} break;
	}

	if (retval != WM_UI_HANDLER_CONTINUE) {
		return retval;
	}

	switch (evt->type) {
		case WINDEACTIVATE: {
			button_activate_state(C, but, BUTTON_STATE_EXIT);
		} break;
	}

	return retval;
}

ROSE_STATIC int ui_handle_button_over(struct rContext *C, const wmEvent *evt, ARegion *region) {
	uiBut *but = ui_but_find_mouse_over_ex(region, evt->mouse_xy);
	if (but) {
		switch (but->type) {
			case UI_BTYPE_EDIT: {
				ui_but_activate(C, region, but, BUTTON_STATE_TEXT_SELECTING);
			} break;
			default: {
				ui_but_activate(C, region, but, BUTTON_STATE_HIGHLIGHT);
			} break;
		}
		return ui_handle_button_event(C, evt, but);
	}
	else {
		
	}

	return WM_UI_HANDLER_CONTINUE;
}

ROSE_STATIC int ui_handler_region_menu(struct rContext *C, const wmEvent *evt, void *user_data) {
	uiHandleButtonData *data = (uiHandleButtonData *)user_data;

	if (data && data->region) {
		uiBut *but = ui_region_find_active_but(data->region);
		if (but) {
			ui_handle_button_event(C, evt, but);
		}
	}

	/** This is a modal handler don't allow anything to interfere with us! */
	return WM_UI_HANDLER_BREAK;
}

ROSE_STATIC int ui_region_handler(struct rContext *C, const wmEvent *evt, void *user_data) {
	wmWindow *window = CTX_wm_window(C);
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
	/** We just ensure the handler in the listbase, we don't need to call #ui_region_handler_remove. */
	WM_event_remove_ui_handler(handlers, ui_region_handler, ui_region_handler_remove, NULL, false);
	WM_event_add_ui_handler(NULL, handlers, ui_region_handler, ui_region_handler_remove, NULL, 0);
}

/** \} */
