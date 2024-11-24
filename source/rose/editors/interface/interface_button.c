#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_vector_types.h"
#include "DNA_userdef_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_resource.h"

#include "KER_context.h"

#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "LIB_listbase.h"
#include "LIB_ghash.h"
#include "LIB_math_vector.h"
#include "LIB_string.h"
#include "LIB_rect.h"
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

	switch(type) {
		case UI_BTYPE_BUT: {
			return &theme->tui.wcol_but;
		} break;
		case UI_BTYPE_TXT: {
			return &theme->tui.wcol_txt;
		} break;
		case UI_BTYPE_EDIT: {
			return &theme->tui.wcol_edit;
		} break;
	}

	return &theme->tui.wcol_but;
}

ROSE_INLINE void copy_f4_u4(float a[4], const unsigned char b[4]) {
	a[0] = b[0] / 255.0f;
	a[1] = b[1] / 255.0f;
	a[2] = b[2] / 255.0f;
	a[3] = b[3] / 255.0f;
}

ROSE_INLINE int ui_but_text_font(uiBut *but) {
	return RFT_set_default();
}

ROSE_INLINE void ui_but_text_rect(uiBut *but, const rcti *rect, rcti *client) {
	client->xmin = rect->xmin + UI_TEXT_MARGIN_X;
	client->xmax = rect->xmax - UI_TEXT_MARGIN_X;
	client->ymin = rect->ymin;
	client->ymax = rect->ymax;
	LIB_rcti_sanitize(client);
}

ROSE_INLINE bool ui_but_text_bounds(uiBut *but, const rcti *rect, rcti *content, int offset) {
	int font = ui_but_text_font(but);
	
	rcti client;
	ui_but_text_rect(but, rect, &client);
	RFT_boundbox(font, but->name, offset, content);
	
	content->xmin += client.xmin;
	content->xmax += client.xmin;
	content->ymin = client.ymin;
	content->ymax = client.ymax;
	
	if (!LIB_rcti_inside_rcti(&client, content)) {
		LIB_rcti_isect(&client, content, content);
		return false;
	}
	return true;
}

ROSE_INLINE void ui_draw_but_back(const struct rContext *C, uiBut *but, uiWidgetColors *colors, const rcti *rect) {
	ARegion *region = CTX_wm_region(C);

	float fill[4];
	float border[4];

	copy_f4_u4(fill, (but->flag & UI_HOVER) ? colors->inner_sel : colors->inner);

	if (fill[3] < 1e-3f) {
		return;
	}
	
	copy_f4_u4(border, colors->outline);

	rctf rectf;
	LIB_rctf_rcti_copy(&rectf, rect);
	UI_draw_roundbox_4fv_ex(&rectf, fill, fill, 0, border, 1, colors->roundness);
}

ROSE_INLINE bool ui_draw_but_text_sel(const struct rContext *C, uiBut *but, uiWidgetColors *colors, const rcti *rect) {
	if(but->selend - but->selsta <= 0) {
		return false;
	}

	float fill[4];
	copy_f4_u4(fill, colors->text_sel);
	
	if (fill[3] < 1e-3f) {
		return false;
	}
	
	int font = ui_but_text_font(but);
	
	rcti selection, a, b;
	ui_but_text_bounds(but, rect, &a, but->selsta);
	ui_but_text_bounds(but, rect, &b, but->selend);
	selection.xmin = ROSE_MIN(a.xmax, b.xmax);
	selection.xmax = ROSE_MAX(a.xmax, b.xmax);
	
	int cy = LIB_rcti_cent_y(&a);
	
	selection.ymin = cy - (RFT_height_max(font) * 2) / 3;
	selection.ymax = cy + (RFT_height_max(font) * 2) / 3;

	rctf self;
	LIB_rctf_rcti_copy(&self, &selection);
	UI_draw_roundbox_4fv_ex(&self, fill, fill, 0, NULL, 1, 0);

	return true;
}

ROSE_INLINE void ui_draw_but_text(const struct rContext *C, uiBut *but, uiWidgetColors *colors, const rcti *rect) {
	rcti client;
	ui_but_text_rect(but, rect, &client);

	float text[4];
	copy_f4_u4(text, colors->text);
	
	int font = ui_but_text_font(but);
	RFT_clipping(font, client.xmin, client.ymin, client.xmax, client.ymax);
	RFT_color4f(font, text[0], text[1], text[2], text[3]);
	RFT_enable(font, RFT_CLIPPING);

	GPU_blend(GPU_BLEND_ALPHA);

	if (ELEM(but->type, UI_BTYPE_BUT)) { // Button texts are aligned in the center.
		RFT_position(font, client.xmin, LIB_rcti_cent_y(&client) - RFT_height_max(font) / 2, 0.0f);
		RFT_draw(font, but->name, -1);
	}
	else {
		ROSE_assert(ELEM(but->type, UI_BTYPE_EDIT, UI_BTYPE_TXT));

		ui_draw_but_text_sel(C, but, colors, rect);

		RFT_position(font, client.xmin, LIB_rcti_cent_y(&client) - RFT_height_max(font) / 2, 0.0f);
		RFT_draw(font, but->name, -1);
	}
}

ROSE_INLINE void ui_draw_but_cursor(const struct rContext *C, uiBut *but, uiWidgetColors *colors, const rcti *rect) {
	if(!ui_but_is_editing(but)) {
		return;
	}
	
	int font = ui_but_text_font(but);
	
	rcti bounds;
	if (ui_but_text_bounds(but, rect, &bounds, but->offset)) {
		bounds.xmin = bounds.xmax;
		bounds.xmax = bounds.xmin + 1;
		
		int cy = LIB_rcti_cent_y(&bounds);
		
		bounds.ymin = cy - (RFT_height_max(font) * 2) / 3;
		bounds.ymax = cy + (RFT_height_max(font) * 2) / 3;
		
		Theme *theme = UI_GetTheme();
		
		float cur[4];
		copy_f4_u4(cur, theme->tui.text_cur);
		
		rctf rectf;
		LIB_rctf_rcti_copy(&rectf, &bounds);
		UI_draw_roundbox_4fv_ex(&rectf, cur, cur, 0, NULL, 1, 0);
	}
}

void ui_draw_but(const struct rContext *C, ARegion *region, uiBut *but, const rcti *rect) {
	if (but->type == UI_BTYPE_SEPR) {
		return;
	}

	uiWidgetColors *colors = widget_colors(but->type);
	
	if (colors == NULL) {
		return;
	}

	ui_draw_but_back(C, but, colors, rect);
	ui_draw_but_text(C, but, colors, rect);
	ui_draw_but_cursor(C, but, colors, rect);
}

/** \} */
