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

ROSE_INLINE void ui_draw_but_back(const struct rContext *C, uiBut *but, uiWidgetColors *colors, const rcti *rect) {
	ARegion *region = CTX_wm_region(C);

	float fill[4];

	if ((but->flag & UI_HOVER) != 0) {
		copy_f4_u4(fill, colors->inner_sel);
	}
	else {
		copy_f4_u4(fill, colors->inner);
	}

	if (colors->inner[3] < 1e-3f) {
		return;
	}

	float border[4];
	copy_v4_v4(border, fill);
	mul_v3_fl(border, 0.25f);

	rctf rectf;
	LIB_rctf_rcti_copy(&rectf, rect);

	UI_draw_roundbox_4fv_ex(&rectf, fill, fill, 0, border, 1, colors->roundness);
}

ROSE_INLINE bool ui_draw_but_text_sel(const struct rContext *C, uiBut *but, uiWidgetColors *colors, const rcti *rect) {
	int font = RFT_set_default();
	const int padx = UI_TEXT_MARGIN_X, halfy = RFT_height_max(font) / 2;

	int cx = LIB_rcti_cent_x(rect);
	int cy = LIB_rcti_cent_y(rect);

	rcti boundl, boundr;
	RFT_boundbox(font, but->name, but->selsta, &boundl);
	RFT_boundbox(font, but->name, but->selend, &boundr);
	rcti sel;
	sel.xmin = boundl.xmax + rect->xmin + padx;
	sel.xmax = boundr.xmax + rect->xmin + padx;
	sel.ymax = cy + halfy;
	sel.ymin = cy - halfy;

	if (!LIB_rcti_isect(rect, &sel, NULL)) {
		return false;
	}

	float back[4];
	copy_f4_u4(back, colors->text_sel);

	rctf self;
	LIB_rctf_rcti_copy(&self, &sel);
	UI_draw_roundbox_4fv_ex(&self, back, back, 0, NULL, 1, 0);

	return true;
}

ROSE_INLINE void ui_draw_but_text(const struct rContext *C, uiBut *but, uiWidgetColors *colors, const rcti *rect) {
	int font = RFT_set_default();
	const int padx = UI_TEXT_MARGIN_X, pady = RFT_height_max(font) / 3;

	float text[4];
	copy_f4_u4(text, colors->text);

	RFT_clipping(font, rect->xmin + padx, rect->ymin + pady, rect->xmax - padx, rect->ymax - pady);
	RFT_color4f(font, text[0], text[1], text[2], text[3]);
	RFT_enable(font, RFT_CLIPPING);

	int cx = LIB_rcti_cent_x(rect);
	int cy = LIB_rcti_cent_y(rect);

	rcti bound;
	RFT_boundbox(font, but->name, -1, &bound);

	GPU_blend(GPU_BLEND_ALPHA);

	if (ELEM(but->type, UI_BTYPE_BUT)) { // Button texts are aligned in the center.
		RFT_position(font, cx - LIB_rcti_size_x(&bound) / 2, cy - pady, -1.0f);
		RFT_draw(font, but->name, -1);
	}
	else {
		ROSE_assert(ELEM(but->type, UI_BTYPE_EDIT, UI_BTYPE_TXT));

		ui_draw_but_text_sel(C, but, colors, rect);

		RFT_position(font, rect->xmin + padx, cy - pady, -1.0f);
		RFT_draw(font, but->name, -1);
	}
}

ROSE_INLINE void ui_draw_but_cursor(const struct rContext *C, uiBut *but, uiWidgetColors *colors, const rcti *rect) {
	if(!ui_but_is_editing(but)) {
		return;
	}

	int font = RFT_set_default();
	const int padx = UI_TEXT_MARGIN_X, halfy = RFT_height_max(font) / 2;

	rcti bound;
	RFT_boundbox(font, but->name, but->offset, &bound);

	int cx = LIB_rcti_cent_x(rect);
	int cy = LIB_rcti_cent_y(rect);

	bound.xmin = bound.xmax - bound.xmin + rect->xmin + padx;
	bound.xmax = bound.xmin + 1;

	bound.ymax = cy + halfy;
	bound.ymin = cy - halfy;

	if (!LIB_rcti_isect(rect, &bound, NULL)) {
		return;
	}

	Theme *theme = UI_GetTheme();

	float cursor[4];
	copy_f4_u4(cursor, theme->tui.text_cur);

	rctf boundf;
	LIB_rctf_rcti_copy(&boundf, &bound);
	UI_draw_roundbox_4fv_ex(&boundf, cursor, cursor, 0, NULL, 1, 0);
}

void ui_draw_but(const struct rContext *C, ARegion *region, uiBut *but, const rcti *rect) {
	if (but->type == UI_BTYPE_SEPR) {
		return;
	}

	uiWidgetColors *colors = widget_colors(but->type);

	ui_draw_but_back(C, but, colors, rect);
	ui_draw_but_text(C, but, colors, rect);
	ui_draw_but_cursor(C, but, colors, rect);
}

/** \} */
