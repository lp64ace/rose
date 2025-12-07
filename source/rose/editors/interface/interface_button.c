#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_userdef_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_resource.h"

#include "KER_context.h"

#include "GPU_framebuffer.h"
#include "GPU_immediate.h"
#include "GPU_matrix.h"
#include "GPU_state.h"
#include "GPU_vertex_format.h"

#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_math_vector.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_string_utf.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "RFT_api.h"

#include "interface_intern.h"

/* -------------------------------------------------------------------- */
/** \name UI Button Draw
 * \{ */

ROSE_INLINE uiWidgetColors *widget_colors(int type) {
	Theme *theme = UI_GetTheme();

	switch (type) {
		case UI_BTYPE_HSPR:
		case UI_BTYPE_VSPR: {
			return &theme->tui.wcol_sepr;
		} break;
		case UI_BTYPE_PUSH: {
			return &theme->tui.wcol_but;
		} break;
		case UI_BTYPE_TEXT: {
			return &theme->tui.wcol_txt;
		} break;
		case UI_BTYPE_EDIT: {
			return &theme->tui.wcol_edit;
		} break;
		case UI_BTYPE_SCROLL: {
			return &theme->tui.wcol_scroll;
		} break;
	}

	return &theme->tui.wcol_but;
}

int ui_but_text_font(uiBut *but) {
	return RFT_set_default();
}

ROSE_INLINE void ui_but_text_rect(uiBut *but, const rcti *rect, rcti *client) {
	client->xmin = rect->xmin + UI_TEXT_MARGIN_X;
	client->xmax = rect->xmax - UI_TEXT_MARGIN_X;
	client->ymin = rect->ymin;
	client->ymax = rect->ymax;
	LIB_rcti_sanitize(client);
}

ROSE_INLINE const char *ui_but_text(uiBut *but) {
	return (but->drawstr) ? but->drawstr : "(null)";
}

ROSE_STATIC void ui_text_clip_give_prev_off(uiBut *but, const char *str) {
	const char *prev_utf8 = LIB_str_find_prev_char_utf8(str + but->scroll, str);
	const int bytes = str + but->scroll - prev_utf8;

	but->scroll -= bytes;
}

ROSE_STATIC void ui_text_clip_give_next_off(uiBut *but, const char *str, const char *str_end) {
	const char *next_utf8 = LIB_str_find_next_char_utf8(str + but->scroll, str_end);
	const int bytes = next_utf8 - (str + but->scroll);

	but->scroll += bytes;
}

/** Updates the scrolling of the edit-button so that the cursor is visible. */
ROSE_STATIC void ui_text_clip_cursor(uiBut *but, const rcti *rect) {
	rcti client;
	ui_but_text_rect(but, rect, &client);
	const int cwidth = ROSE_MAX(0, LIB_rcti_size_x(&client));

	const char *str = ui_but_text(but);
	if (!str || cwidth <= 20) {
		return;
	}

	if (but->scroll > but->offset) {
		but->scroll = but->offset;
	}

	int font = ui_but_text_font(but);

	if (RFT_width(font, str, INT_MAX) <= cwidth) {
		but->scroll = 0;
	}

	but->strwidth = RFT_width(font, str + but->scroll, INT_MAX);

	if (but->strwidth > cwidth) {
		unsigned int length = LIB_strlen(str);
		unsigned int adjusted = length;

		while (but->strwidth > cwidth) {
			float width = RFT_width(font, str + but->scroll, but->offset - but->scroll);

			if (width > cwidth - 20) {
				ui_text_clip_give_next_off(but, str, str + length);
			}
			else {
				if (width < 20 && but->scroll > 0) {
					ui_text_clip_give_prev_off(but, str);
				}
				adjusted -= LIB_str_utf8_size_safe(LIB_str_find_prev_char_utf8(str + adjusted, str));
			}

			but->strwidth = RFT_width(font, str + but->scroll, adjusted - but->scroll);

			if (but->strwidth < 20) {
				break;
			}
		}
	}
}

ROSE_STATIC void ui_draw_separator_ex(const rcti *rect, bool vertical, const unsigned char color[4]) {
	const int mid = vertical ? LIB_rcti_cent_x(rect) : LIB_rcti_cent_y(rect) + 1;

	const unsigned int pos = GPU_vertformat_add(immVertexFormat(), "pos", GPU_COMP_F32, 2, GPU_FETCH_FLOAT);
	immBindBuiltinProgram(GPU_SHADER_3D_UNIFORM_COLOR);

	GPU_blend(GPU_BLEND_ALPHA);
	GPU_line_width(1.0f);
	
	immUniformColor4ubv(color);
	immBegin(GPU_PRIM_LINES, 2);
	if (vertical) {
		immVertex2f(pos, mid, rect->ymin);
		immVertex2f(pos, mid, rect->ymax);
	}
	else {
		immVertex2f(pos, rect->xmin, mid);
		immVertex2f(pos, rect->xmax, mid);
	}
	immEnd();
	
	GPU_blend(GPU_BLEND_NONE);

	immUnbindProgram();
}

ROSE_STATIC void ui_draw_separator(const uiWidgetColors *colors, uiBut *but, const rcti *rect) {
	ui_draw_separator_ex(rect, but->type == UI_BTYPE_VSPR, colors->text);
}

ROSE_STATIC float ui_widget_roundness(uiWidgetColors *wcol, const rcti *rect) {
	return wcol->roundness * ROSE_MIN(LIB_rcti_size_x(rect), LIB_rcti_size_y(rect));
}

ROSE_STATIC bool ui_draw_but_row_extended_left(uiBut *but) {
	if (but->draw & UI_BUT_GRID) {
		uiBut *left = but->prev;
		if (left && left->draw & UI_BUT_GRID && DRAW_INDX(left->draw) == DRAW_INDX(but->draw)) {
			/**
			 * There is a case where the previous button will belong to a different list but have the same index, 
			 * for example when both button are in the first row and both lists have a single row.
			 * The following is a temporary solution to tackle that issue since it only works for lists that are 
			 * not next to each other, for example the following layout would still have issues:
			 * > LST1/ROW1 LST2/ROW1
			 * But the following will work:
			 * > LST1/ROW1
			 * > LST2/ROW1
			 *
			 * Later on lists will probably have a separate pointer a special list structure that will handle list events, 
			 * such as move/resize/rename/select/deselect/scroll items.
			 */
			return left->rect.ymin == but->rect.ymin && left->rect.ymax == but->rect.ymax;
		}
	}
	return false;
}

ROSE_STATIC bool ui_draw_but_row_extended_right(uiBut *but) {
	if ((but->draw & UI_BUT_GRID) != 0 && (but->draw & UI_BUT_ROW) != 0) {
		uiBut *right = but->next;
		if (right && right->draw & UI_BUT_GRID && DRAW_INDX(right->draw) == DRAW_INDX(but->draw)) {
			/**
			 * There is a case where the previous button will belong to a different list but have the same index, 
			 * for example when both button are in the first row and both lists have a single row.
			 * The following is a temporary solution to tackle that issue since it only works for lists that are 
			 * not next to each other, for example the following layout would still have issues:
			 * > LST1/ROW1 LST2/ROW1
			 * But the following will work:
			 * > LST1/ROW1
			 * > LST2/ROW1
			 *
			 * Later on lists will probably have a separate pointer a special list structure that will handle list events, 
			 * such as move/resize/rename/select/deselect/scroll items.
			 */
			return right->rect.ymin == but->rect.ymin && right->rect.ymax == but->rect.ymax;
		}
	}
	return false;
}

ROSE_STATIC bool ui_draw_but_row_hovered(uiBut *but) {
	if ((but->draw & UI_BUT_GRID) != 0 && (but->draw & UI_BUT_ROW) != 0) {
		int row = DRAW_INDX(but->draw);
		for (uiBut *l = but->prev; l && ui_draw_but_row_extended_left(l->next); l = l->prev) {
			if (l->flag & UI_HOVER) {
				return true;
			}
		}
		for (uiBut *r = but->next; r && ui_draw_but_row_extended_right(r->prev); r = r->next) {
			if (r->flag & UI_HOVER) {
				return true;
			}
		}
		return but->flag & UI_HOVER;
	}
	return false;
}

ROSE_STATIC void ui_draw_text_back(uiWidgetColors *wcol, uiBut *but, const rcti *recti, bool selected) {
	selected |= ui_draw_but_row_hovered(but);
	/** Remove the left or right roundbox if we are rendering a list. */
	if (but->draw & UI_BUT_GRID) {
		int lcorner = selected && !ui_draw_but_row_extended_left(but) ? UI_CNR_TOP_LEFT | UI_CNR_BOTTOM_LEFT : UI_CNR_NONE;
		int rcorner = selected && !ui_draw_but_row_extended_right(but) ? UI_CNR_TOP_RIGHT | UI_CNR_BOTTOM_RIGHT : UI_CNR_NONE;
		UI_draw_roundbox_corner_set(0 /* lcorner | rcorner */);
	}
	const unsigned char *inner = selected ? wcol->inner_sel : wcol->inner;

	unsigned char alpha = inner[3];
	/** Add a little background for items that are in a list. */
	if (!selected && (but->draw & UI_BUT_GRID) != 0 && (but->draw & UI_BUT_ROW)) {
		alpha = (wcol->inner_sel[3] + wcol->inner[3]) / (2 + DRAW_INDX(but->draw) % 2);
	}

	if (alpha > 1e-3f) {
		rctf rect;
		LIB_rctf_rcti_copy(&rect, recti);
		UI_draw_roundbox_3ub_alpha(&rect, true, ui_widget_roundness(wcol, recti), inner, alpha);
	}
	
	UI_draw_roundbox_corner_set(UI_CNR_ALL);
}

ROSE_STATIC void ui_draw_text_font_clip_begin(int font, const rcti *rect) {
	RFT_enable(font, RFT_CLIPPING);
	RFT_clipping(font, rect->xmin, rect->ymin, rect->xmax, rect->ymax);
}

ROSE_STATIC void ui_draw_text_font_clip_end(int font) {
	RFT_disable(font, RFT_CLIPPING);
}

ROSE_STATIC void ui_draw_text_font_position(uiBut *but, int font, const rcti *rect, const char *text, int *r_x, int *r_y) {
	if (ELEM(but->type, UI_BTYPE_TEXT, UI_BTYPE_PUSH, UI_BTYPE_MENU, UI_BTYPE_EDIT)) {
		*r_y = LIB_rcti_cent_y(rect) - RFT_height_max(font) / 3;
	}
	else {
		*r_y = rect->ymax - RFT_height_max(font);
	}
	if (but->draw & UI_BUT_TEXT_LEFT) {
		*r_x = rect->xmin;
	}
	else {
		float width = ROSE_MIN(RFT_width(font, text, -1), LIB_rcti_size_x(rect));
		*r_x = (but->draw & UI_BUT_TEXT_RIGHT) ? rect->xmax - width : LIB_rcti_cent_x(rect) - width / 2;
	}
	RFT_position(font, *r_x, *r_y, 0);
}

ROSE_STATIC void ui_draw_text_font_caret(uiBut *but, const rcti *recti, int font, const char *text, int x, int y) {
	if (ui_but_is_editing(but)) {
		Theme *theme = UI_GetTheme();

		rcti rect;
		rect.xmin = rect.xmax = x + RFT_width(font, text, but->offset - but->scroll);
		rect.ymin = y - (RFT_height_max(font) * 1) / 4;
		rect.ymax = y + (RFT_height_max(font) * 3.25) / 4;
		
		ui_draw_separator_ex(&rect, true, theme->tui.text_cur);
	}
}

ROSE_STATIC void ui_draw_text_font_draw(uiWidgetColors *wcol, uiBut *but, const rcti *recti, int font, const char *text, int x, int y) {
	if (ELEM(but->type, UI_BTYPE_EDIT)) {
		rctf rect;
		rect.xmin = ROSE_MIN(recti->xmax, x + RFT_width(font, text, ROSE_MAX(0, but->selsta - but->scroll)));
		rect.xmax = ROSE_MIN(recti->xmax, x + RFT_width(font, text, ROSE_MAX(0, but->selend - but->scroll)));
		rect.ymin = y - (RFT_height_max(font) * 1) / 4 - PIXELSIZE;
		rect.ymax = y + (RFT_height_max(font) * 3.25) / 4 + PIXELSIZE;
		UI_draw_roundbox_3ub_alpha(&rect, true, 0, wcol->text_sel, wcol->text_sel[3]);
	}

	RFT_color4ubv(font, wcol->text);
	RFT_draw(font, text, -1);
}

ROSE_STATIC void ui_draw_text_font(uiWidgetColors *wcol, uiBut *but, const rcti *recti, int font, const char *text) {
	rcti rect;
	ui_text_clip_cursor(but, recti);
	ui_but_text_rect(but, recti, &rect);

	int x, y;
	ui_draw_text_font_clip_begin(font, &rect);
	ui_draw_text_font_position(but, font, &rect, text + but->scroll, &x, &y);
	ui_draw_text_font_draw(wcol, but, &rect, font, text + but->scroll, x, y);
	ui_draw_text_font_caret(but, &rect, font, text + but->scroll, x, y);
	ui_draw_text_font_clip_end(font);
}

ROSE_STATIC void ui_draw_scroll_thumb(uiWidgetColors *wcol, uiBut *but, const rcti *recti, bool selected) {
	selected |= ui_draw_but_row_hovered(but);
	/** Remove the left or right roundbox if we are rendering a list. */
	if (but->draw & UI_BUT_GRID) {
		int lcorner = selected && !ui_draw_but_row_extended_left(but) ? UI_CNR_TOP_LEFT | UI_CNR_BOTTOM_LEFT : UI_CNR_NONE;
		int rcorner = selected && !ui_draw_but_row_extended_right(but) ? UI_CNR_TOP_RIGHT | UI_CNR_BOTTOM_RIGHT : UI_CNR_NONE;
		UI_draw_roundbox_corner_set(/* lcorner | rcorner */ UI_CNR_NONE);
	}
	const unsigned char *inner = selected ? wcol->text_sel : wcol->text;

	unsigned char alpha = inner[3];
	/** Add a little background for items that are in a list. */
	if (!selected && (but->draw & UI_BUT_GRID) != 0 && (but->draw & UI_BUT_ROW)) {
		alpha = (wcol->text_sel[3] + wcol->text[3]) / (2 + DRAW_INDX(but->draw) % 2);
	}

	if (alpha > 1e-3f) {
		double value;
		ui_but_value_get(but, &value);

		double thumby = (but->softmax - value) / (but->softmax - but->softmin + 1);
		double thumbr = 1.0 / (but->softmax - but->softmin + 1);

		rctf rect;
		LIB_rctf_rcti_copy(&rect, recti);

		rect.ymin = recti->ymin + (thumby + 0.0) * LIB_rcti_size_y(recti);
		rect.ymax = recti->ymin + (thumby + thumbr) * LIB_rcti_size_y(recti);

		UI_draw_roundbox_3ub_alpha(&rect, true, ui_widget_roundness(wcol, recti), inner, alpha);
	}

	UI_draw_roundbox_corner_set(UI_CNR_ALL);
}

ROSE_STATIC void ui_draw_text(uiWidgetColors *wcol, uiBut *but, const rcti *recti) {
	bool selected = ui_but_is_editing(but);
	if (ELEM(but->type, UI_BTYPE_EDIT)) {
		selected |= (but->flag & UI_HOVER) != 0;
	}
	ui_draw_text_back(wcol, but, recti, selected);
	ui_draw_text_font(wcol, but, recti, ui_but_text_font(but), ui_but_text(but));
}

ROSE_STATIC void ui_draw_scroll(uiWidgetColors *wcol, uiBut *but, const rcti *recti) {
	bool in_menu = but->block->handle != NULL;
	/** menu items do not get a background when not hovered! */
	if (!in_menu || (but->flag & UI_HOVER) != 0) {
		ui_draw_text_back(wcol, but, recti, (but->flag & UI_HOVER) != 0);
	}
	ui_draw_scroll_thumb(wcol, but, recti, (but->flag & UI_HOVER) != 0);
}

ROSE_STATIC void ui_draw_widget(uiWidgetColors *wcol, uiBut *but, const rcti *recti) {
	bool in_menu = but->block->handle != NULL;
	/** menu items do not get a background when not hovered! */
	if (!in_menu || (but->flag & UI_HOVER) != 0) {
		ui_draw_text_back(wcol, but, recti, (but->flag & UI_HOVER) != 0);
	}
	ui_draw_text_font(wcol, but, recti, ui_but_text_font(but), ui_but_text(but));
}

void ui_draw_but(const struct rContext *C, ARegion *region, uiBut *but, const rcti *rect) {
	if (but->type == UI_BTYPE_SEPR) {
		return;
	}

	uiWidgetColors *colors = widget_colors(but->type);

	if (colors == NULL) {
		return;
	}

	uiWidgetColors mcolors;
	memcpy(&mcolors, colors, sizeof(uiWidgetColors));

	if (but->flag & UI_DISABLED) {
		mcolors.inner[3] = mcolors.inner[3] * 2 / 3;
		mcolors.text[3] = mcolors.text[3] * 2 / 3;
	}

	switch (but->type) {
		case UI_BTYPE_HSPR:
		case UI_BTYPE_VSPR: {
			ui_draw_separator(&mcolors, but, rect);
		} break;
		case UI_BTYPE_TEXT:
		case UI_BTYPE_EDIT: {
			ui_draw_text(&mcolors, but, rect);
		} break;
		case UI_BTYPE_SCROLL: {
			ui_draw_scroll(&mcolors, but, rect);
		} break;
		default: {
			ui_draw_widget(&mcolors, but, rect);
		} break;
	}
}

/** \} */
