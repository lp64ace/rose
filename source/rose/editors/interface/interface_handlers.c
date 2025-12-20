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
#include "LIB_math_base.h"
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

ROSE_STATIC int ui_region_handler(rContext *C, const wmEvent *evt, void *user_data);

typedef struct uiHandleButtonData {
	struct wmWindow *window;
	struct ScrArea *area;
	struct ARegion *region;

	float value;

	int state;

	int selini;

	struct uiUndoStack_Text *undo_stack;
	struct uiPopupBlockHandle *menu;
} uiHandleButtonData;

/** #uiHandleButtonData->state */
enum {
#define _BUTTON_STATE_BEGIN BUTTON_STATE_HIGHLIGHT
#define _BUTTON_MODAL_STATE_BEGIN BUTTON_STATE_HIGHLIGHT
	BUTTON_STATE_HIGHLIGHT = 1,
	BUTTON_STATE_TEXT_SELECTING,
	BUTTON_STATE_TEXT_EDITING,
	BUTTON_STATE_WAIT_RELEASE,
	BUTTON_STATE_MENU_OPEN,
	BUTTON_STATE_SCROLL,
#define _BUTTON_MODAL_STATE_END BUTTON_STATE_SCROLL
	BUTTON_STATE_EXIT,
#define _BUTTON_STATE_END BUTTON_STATE_EXIT
};

bool ui_but_is_editing(uiBut *but) {
	uiHandleButtonData *data = but->active;
	return (data && data->state == BUTTON_STATE_TEXT_EDITING);
}

void ui_but_value_set(uiBut *but, double value) {
	if (but->rna_property) {
		PropertyRNA *property = but->rna_property;

		if (RNA_property_editable(&but->rna_pointer, property)) {
			switch (RNA_property_type(property)) {
				case PROP_FLOAT: {
					if (RNA_property_array_check(property)) {
						RNA_property_float_set_index(&but->rna_pointer, property, but->rna_index, value);
					}
				} break;
				default: {
					ROSE_assert_unreachable();
				} break;
			}
		}
	}
	else if (but->pointype != UI_POINTER_NIL) {
		switch (but->pointype) {
			case UI_POINTER_BYTE: {
				value = round_db_to_uchar_clamp(value);
			} break;
			case UI_POINTER_INT: {
				value = round_db_to_int_clamp(value);
			} break;
			case UI_POINTER_FLT: {
				float fval = value;
				if (fval >= -0.00001f && fval <= 0.00001f) {
					/* prevent negative zero */
					fval = 0.0f;
				}
				value = fval;
			} break;
		}

		switch (but->pointype) {
			case UI_POINTER_BYTE: {
				*((unsigned char *)but->poin) = (unsigned char)(value);
			} break;
			case UI_POINTER_INT: {
				*((int *)but->poin) = (int)(value);
			} break;
			case UI_POINTER_FLT: {
				*((float *)but->poin) = (float)(value);
			} break;
		}
	}
}

void ui_but_value_get(uiBut *but, double *r_value) {
	double value = 0.0;

	if (but->poin == NULL && but->rna_pointer.data == NULL) {
		if (r_value) {
			*r_value = value;
		}
		return;
	}

	if (but->rna_pointer.data) {
		PropertyRNA *property = but->rna_property;

		ROSE_assert(but->rna_index != -1);

		switch (RNA_property_type(property)) {
			case PROP_FLOAT: {
				if (RNA_property_array_check(property)) {
					value = RNA_property_float_get_index(&but->rna_pointer, property, but->rna_index);
				}
				else {
					value = RNA_property_float_get(&but->rna_pointer, property);
				}
			} break;
			default: {
				ROSE_assert_unreachable();
			} break;
		}
	}
	else if (but->pointype == UI_POINTER_BYTE) {
		value = *(char *)but->poin;
	}
	else if (but->pointype == UI_POINTER_INT) {
		value = *(int *)but->poin;
	}
	else if (but->pointype == UI_POINTER_FLT) {
		value = *(float *)but->poin;
	}

	if (r_value) {
		*r_value = value;
	}
}

/* avoid unneeded calls to ui_but_value_get */
#define UI_BUT_VALUE_UNSET DBL_MAX
#define UI_GET_BUT_VALUE_INIT(_but, _value) \
	if (_value == DBL_MAX) {                \
		ui_but_value_get(_but, &(_value));  \
	}                                       \
	((void)0)

ROSE_INLINE bool ui_but_is_float(uiBut *but) {
	if (but->pointype == UI_POINTER_FLT && but->poin) {
		return true;
	}
	if (but->rna_property && RNA_property_type(but->rna_property) == PROP_FLOAT) {
		return true;
	}

	return false;
}

/* Maximum number of digits of precision (not number of decimal places)
 * to display for float values. Note that the UI_FLOAT_VALUE_DISPLAY_*
 * defines that follow depend on this. */
#define UI_PRECISION_FLOAT_MAX 6

ROSE_INLINE int ui_calc_float_precision(int precision, double value) {
	static const double pow10_neg[UI_PRECISION_FLOAT_MAX + 1] = {1e0, 1e-1, 1e-2, 1e-3, 1e-4, 1e-5, 1e-6};
	static const double max_pow = 10000000.0; /* pow(10, UI_PRECISION_FLOAT_MAX) */

	ROSE_assert(precision <= UI_PRECISION_FLOAT_MAX);
	ROSE_assert(fabs(pow10_neg[precision] - pow(10, -precision)) < 1e-16);

	/* Check on the number of decimal places need to display the number,
	 * this is so 0.00001 is not displayed as 0.00,
	 * _but_, this is only for small values as 10.0001 will not get the same treatment.
	 */
	value = fabs(value);
	if ((value < pow10_neg[precision]) && (value > (1.0 / max_pow))) {
		int value_i = round(value * max_pow);
		if (value_i != 0) {
			const int prec_span = 3; /* show: 0.01001, 5 would allow 0.0100001 for eg. */
			int test_prec;
			int prec_min = -1;
			int dec_flag = 0;
			int i = UI_PRECISION_FLOAT_MAX;
			while (i && value_i) {
				if (value_i % 10) {
					dec_flag |= 1 << i;
					prec_min = i;
				}
				value_i /= 10;
				i--;
			}

			/* even though its a small value, if the second last digit is not 0, use it */
			test_prec = prec_min;

			dec_flag = (dec_flag >> (prec_min + 1)) & ((1 << prec_span) - 1);

			while (dec_flag) {
				test_prec++;
				dec_flag = dec_flag >> 1;
			}

			precision = ROSE_MAX(test_prec, precision);
		}
	}

	CLAMP(precision, 0, UI_PRECISION_FLOAT_MAX);

	return precision;
}

ROSE_INLINE int ui_but_get_float_precision(uiBut *but) {
	if (but->type == UI_BTYPE_TEXT || but->type == UI_BTYPE_EDIT) {
		return but->precision;
	}
	return 1.0f;
}

ROSE_INLINE int ui_but_calc_float_precision(uiBut *but, double value) {
	int precision = ui_but_get_float_precision(but);

	if (precision < 0) {
		precision = (but->hardmax < 10) ? 3 : 2;
	}
	return ui_calc_float_precision(precision, value);

}

void ui_but_update(uiBut *but, bool validate) {
	double value = UI_BUT_VALUE_UNSET;

	switch (but->type) {
		case UI_BTYPE_SCROLL: {
			if (validate) {
				UI_GET_BUT_VALUE_INIT(but, value);
				if (value < but->hardmin) {
					ui_but_value_set(but, but->hardmin);
				}
				else if (value > but->hardmax) {
					ui_but_value_set(but, but->hardmax);
				}

				/* max must never be smaller than min! Both being equal is allowed though */
				ROSE_assert(but->softmin <= but->softmax && but->hardmin <= but->hardmax);
			}
		} break;
	}

	switch (but->type) {
		case UI_BTYPE_TEXT: {
			if (ui_but_is_float(but)) {
				UI_GET_BUT_VALUE_INIT(but, value);
				const int precision = ui_but_calc_float_precision(but, value);

				LIB_strnformat(but->drawstr, but->maxlength, "%.*f", precision, (float)value);
			}
		} break;
	}
}

ROSE_INLINE bool button_modal_state(int state) {
	return IN_RANGE_INCL(state, _BUTTON_MODAL_STATE_BEGIN, _BUTTON_MODAL_STATE_END);
}
ROSE_INLINE bool ui_button_modal_state(uiBut *but) {
	if (!but) {
		return false;
	}
	uiHandleButtonData *data = but->active;
	if (!data) {
		return false;
	}
	return button_modal_state(data->state);
}

ROSE_STATIC int ui_handler_region_menu(rContext *C, const wmEvent *evt, void *user_data);

void ui_do_but_activate_init(rContext *C, ARegion *region, uiBut *but, int state);
void ui_do_but_activate_exit(rContext *C, ARegion *region, uiBut *but);

ROSE_STATIC void ui_but_tag_redraw(uiBut *but) {
	uiHandleButtonData *data = but->active;
	if (!data) {
		return;
	}

	if (data->menu) {
		ED_region_tag_redraw(data->menu->region);
	}
	ED_region_tag_redraw_no_rebuild(data->region);
}

ROSE_STATIC void button_activate_state(rContext *C, uiBut *but, int state) {
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

	if (ELEM(state, BUTTON_STATE_MENU_OPEN)) {
		if (but->menu_create_func) {
			data->menu = ui_popup_block_create(C, data->region, but, but->menu_create_func, NULL, but->arg);
		}
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

void ui_do_but_activate_init(rContext *C, ARegion *region, uiBut *but, int state) {
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
			data->selini = -1;
			data->undo_stack = ui_textedit_undo_stack_create();
			if (state == BUTTON_STATE_TEXT_EDITING) {
				but->selsta = 0;
				but->selend = but->offset = LIB_strlen(but->drawstr);
			}
			ui_textedit_undo_push(data->undo_stack, but->drawstr, LIB_strlen(but->drawstr));
		} break;
	}
}

void ui_do_but_activate_exit(rContext *C, ARegion *region, uiBut *but) {
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

void ui_but_active_free(rContext *C, uiBut *but) {
	if (but->active) {
		uiHandleButtonData *data = but->active;
		button_activate_state(C, but, BUTTON_STATE_EXIT);
	}
}

ROSE_STATIC void ui_but_activate(rContext *C, ARegion *region, uiBut *but, int state) {
	uiBut *old = ui_region_find_active_but(region);
	if (old) {
		uiHandleButtonData *data = old->active;
		button_activate_state(C, old, BUTTON_STATE_EXIT);
	}
	ui_do_but_activate_init(C, region, but, state);
}

ROSE_STATIC void ui_do_but_menu_exit(rContext *C, uiBut *but) {
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

ROSE_STATIC void ui_apply_but_func(rContext *C, uiBut *but, void *arg1, void *arg2) {
	if (but->handle_func) {
		but->handle_func(C, but, arg1, arg2);
	}

	/** We are currently inside a popup menu and a button was handled, we need to exit the menu! */
	if (but->block && but->block->handle) {
		ui_do_but_menu_exit(C, but);
	}
}

ROSE_STATIC bool ui_scroll_set_thumb_pos(uiBut *but, const ARegion *region, const int xy[2]) {
	float mx = xy[0], my = xy[1];
	ui_window_to_block_fl(region, but->block, &mx, &my);

	float mv = 0;
	switch (but->type) {
		case UI_BTYPE_SCROLL: {
			mv = ((but->rect.ymax - my) / LIB_rctf_size_y(&but->rect));
		} break;
		default: {
			ROSE_assert_unreachable();
		} break;
	}
	mv = but->softmin + mv * (but->softmax - but->softmin + 1) - 0.5;

	double old_value, new_value;
	ui_but_value_get(but, &old_value);
	ui_but_value_set(but, mv);
	ui_but_value_get(but, &new_value);

	return old_value != new_value;
}

ROSE_STATIC int ui_do_but(rContext *C, uiBlock *block, uiBut *but, const wmEvent *evt) {
	int retval = WM_UI_HANDLER_CONTINUE;

	ARegion *region = CTX_wm_region(C);
	if (but->active && ((uiHandleButtonData *)but->active)->region) {
		region = ((uiHandleButtonData *)but->active)->region;
	}

	switch (but->type) {
		case UI_BTYPE_SCROLL: {
			if (ELEM(evt->type, WHEELDOWNMOUSE, WHEELUPMOUSE, EVT_PAGEDOWNKEY, EVT_PAGEUPKEY) && ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
				double move = ELEM(evt->type, WHEELUPMOUSE, EVT_PAGEUPKEY) ? -15e-2f : 15e-2f;

				double old_value, new_value;
				ui_but_value_get(but, &old_value);
				ui_but_value_set(but, old_value + move);
				ui_but_value_get(but, &new_value);

				if (old_value != new_value) {
					ED_region_tag_redraw(region);
					// ui_apply_but_func(C, but, but->arg1, but->arg2);
				}

				retval |= WM_UI_HANDLER_BREAK;
			}
			if (ELEM(evt->type, EVT_ENDKEY, EVT_HOMEKEY) && ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
				double nval = ELEM(evt->type, EVT_HOMEKEY) ? but->softmin : but->softmax;
				
				double old_value, new_value;
				ui_but_value_get(but, &old_value);
				ui_but_value_set(but, nval);
				ui_but_value_get(but, &new_value);

				if (old_value != new_value) {
					ED_region_tag_redraw(region);
					// ui_apply_but_func(C, but, but->arg1, but->arg2);
				}

				retval |= WM_UI_HANDLER_BREAK;
			}
		} break;
		case UI_BTYPE_PUSH: {
			if (ELEM(evt->type, EVT_RETKEY) && ELEM(evt->value, KM_PRESS, KM_DBL_CLICK)) {
				ui_apply_but_func(C, but, but->arg1, but->arg2);
			}
		} break;
	}

	return retval;
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

ROSE_STATIC bool ui_textedit_insert_buf(rContext *C, uiBut *but, const char *input, unsigned int step) {
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
			if (but->handle_text_func == NULL || but->handle_text_func(C, but, nbuf)) {
				SWAP(char *, but->drawstr, nbuf);

				but->offset += step;
				changed |= true;
			}
			MEM_freeN(nbuf);
		}
	}

	return changed;
}

enum {
	UI_TEXTEDIT_PASTE = 1,
	UI_TEXTEDIT_COPY,
	UI_TEXTEDIT_CUT,
};

ROSE_STATIC bool ui_textedit_copypaste(rContext *C, uiBut *but, const int mode) {
	bool changed = false;

	unsigned int length = 0;
	switch (mode) {
		case UI_TEXTEDIT_PASTE: {
			char *nbuf = WM_clipboard_text_get_firstline(C, false, &length);
			if (nbuf) {
				changed |= ui_textedit_insert_buf(C, but, nbuf, length);
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

ROSE_STATIC bool ui_but_is_editing_set(uiBut *but, const char *text) {
	if (text) {
		MEM_freeN(but->drawstr);
		but->drawstr = LIB_strdupN(text);
		but->selsta = but->selend = but->offset = LIB_strlen(but->drawstr);
		ui_but_tag_redraw(but);
		return true;
	}
	return false;
}

ROSE_STATIC int ui_do_but_textsel(rContext *C, uiBlock *block, uiBut *but, uiHandleButtonData *data, const wmEvent *evt) {
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
	if (ELEM(but->type, UI_BTYPE_SEPR, UI_BTYPE_HSPR, UI_BTYPE_VSPR)) {
		return NULL;
	}

	for (uiBut *other = but->next; other; other = other->next) {
		if (ui_but_is_editable_as_text(other)) {
			return other;
		}
	}
	for (uiBut *other = (uiBut *)block->buttons.first; other; other = other->next) {
		if (ui_but_is_editable_as_text(other)) {
			return other;
		}
	}

	return NULL;
}

ROSE_STATIC uiBut *ui_textedit_prev_but(uiBlock *block, uiBut *but, uiHandleButtonData *data) {
	if (ELEM(but->type, UI_BTYPE_SEPR, UI_BTYPE_HSPR, UI_BTYPE_VSPR)) {
		return NULL;
	}

	for (uiBut *other = but->prev; other; other = other->prev) {
		if (ui_but_is_editable_as_text(other)) {
			return other;
		}
	}
	for (uiBut *other = (uiBut *)block->buttons.last; other; other = other->prev) {
		if (ui_but_is_editable_as_text(other)) {
			return other;
		}
	}

	return NULL;
}

ROSE_STATIC int ui_do_but_textedit(rContext *C, uiBlock *block, uiBut *but, uiHandleButtonData *data, const wmEvent *evt) {
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
			button_activate_state(C, but, BUTTON_STATE_EXIT);
			ui_apply_but_func(C, but, but->arg1, but->arg2);
			retval |= WM_UI_HANDLER_BREAK;
		} break;
		case EVT_ESCKEY: {
			button_activate_state(C, but, BUTTON_STATE_EXIT);
			retval |= WM_UI_HANDLER_BREAK;
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
					if (ui_but_is_editing_set(but, text)) {
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
			changed |= ui_textedit_insert_buf(C, but, evt->input, LIB_str_utf8_size_or_error(evt->input));
		}
	}

	if (changed) {
		ui_textedit_undo_push(data->undo_stack, but->drawstr, but->offset);
		ui_but_tag_redraw(but);
		retval |= WM_UI_HANDLER_BREAK;
	}

	return retval;
}

ROSE_STATIC int ui_handle_button_event(rContext *C, const wmEvent *evt, uiBut *but);

ROSE_STATIC int ui_handle_button_over(rContext *C, const wmEvent *evt, ARegion *region) {
	uiBut *but = ui_but_find_mouse_over_ex(region, evt->mouse_xy);
	if (but) {
		switch (but->type) {
			default: {
				ui_but_activate(C, region, but, BUTTON_STATE_HIGHLIGHT);
			} break;
		}
		return ui_handle_button_event(C, evt, but);
	}

	return WM_UI_HANDLER_CONTINUE;
}

ROSE_STATIC int ui_handle_button_event(rContext *C, const wmEvent *evt, uiBut *but) {
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
					switch (but->type) {
						case UI_BTYPE_SCROLL: {
							button_activate_state(C, but, BUTTON_STATE_SCROLL);
						} break;
						case UI_BTYPE_EDIT: {
							ui_textedit_cursor_select(but, data, evt->mouse_xy[0]);
							if (data->selini < 0) {
								data->selini = but->offset;
							}
							button_activate_state(C, but, BUTTON_STATE_TEXT_SELECTING);
						} break;
						default: {
							button_activate_state(C, but, BUTTON_STATE_WAIT_RELEASE);
						} break;
					}
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
			if (evt->type == LEFTMOUSE && ELEM(evt->value, KM_RELEASE)) {
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
		case BUTTON_STATE_SCROLL: {
			if (ui_scroll_set_thumb_pos(but, data->region, evt->mouse_xy)) {
				ED_region_tag_redraw(data->region);
				ui_apply_but_func(C, but, but->arg1, but->arg2);
			}

			retval |= WM_UI_HANDLER_BREAK;

			if (evt->type == LEFTMOUSE && evt->value == KM_RELEASE) {
				button_activate_state(C, but, BUTTON_STATE_EXIT);
			}
		} break;
	}

	if (retval != WM_UI_HANDLER_CONTINUE) {
		return retval;
	}

	retval |= ui_do_but(C, but->block, but, evt);

	switch (evt->type) {
		case WINDEACTIVATE: {
			button_activate_state(C, but, BUTTON_STATE_EXIT);
		} break;
	}

	return retval;
}

ROSE_STATIC int ui_handler_region_menu(rContext *C, const wmEvent *evt, void *user_data) {
	uiHandleButtonData *data = (uiHandleButtonData *)user_data;

	int retval = WM_UI_HANDLER_CONTINUE;

	if (data && data->region) {
		uiBut *but = ui_region_find_active_but(data->region);
		if (but) {
			retval |= ui_handle_button_event(C, evt, but);
		}
	}

	return retval;
}

ROSE_STATIC int ui_region_handler(rContext *C, const wmEvent *evt, void *user_data) {
	wmWindow *window = CTX_wm_window(C);
	ARegion *region = CTX_wm_region(C);
	int retval = WM_UI_HANDLER_CONTINUE;

	if (region == NULL || LIB_listbase_is_empty(&region->uiblocks)) {
		return retval;
	}

	uiBut *but;

	but = ui_region_find_active_but(region);

	if (but) {
		retval |= ui_handle_button_event(C, evt, but);
	}

	if (retval != WM_UI_HANDLER_CONTINUE) {
		return retval;
	}

	if (!ui_button_modal_state(but)) {
		retval |= ui_handle_button_over(C, evt, region);
	}

	if (retval != WM_UI_HANDLER_CONTINUE) {
		return retval;
	}

	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		LISTBASE_FOREACH_BACKWARD(uiBut *, but, &block->buttons) {
			if (but->flag & UI_DEFAULT) {
				retval |= ui_do_but(C, block, but, evt);
			}
			if (retval != WM_UI_HANDLER_CONTINUE) {
				return retval;
			}
		}
	}

	return retval;
}

ROSE_STATIC void ui_region_handler_remove(rContext *C, void *user_data) {
}

void UI_region_handlers_add(ListBase *handlers) {
	/** We just ensure the handler in the listbase, we don't need to call #ui_region_handler_remove. */
	WM_event_remove_ui_handler(handlers, ui_region_handler, ui_region_handler_remove, NULL, false);
	WM_event_add_ui_handler(NULL, handlers, ui_region_handler, ui_region_handler_remove, NULL, 0);
}

/** \} */
