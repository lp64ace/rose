#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_userdef_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_resource.h"
#include "UI_view2d.h"

#include "KER_context.h"

#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_math_vector.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "RFT_api.h"

#include "interface_intern.h"

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name UI Button
 * \{ */

ROSE_STATIC void ui_but_free(struct rContext *C, uiBut *but);

ROSE_INLINE bool ui_but_equals_old(const uiBut *but_new, const uiBut *but_old) {
	if (but_new->type != but_old->type) {
		return false;
	}
	const int runtime = UI_HOVER | UI_SELECT;
	if ((but_new->flag & ~(runtime)) != (but_old->flag & ~(runtime))) {
		return false;
	}
	if (but_new->handle_func != but_old->handle_func || but_new->arg1 != but_old->arg1 || but_new->arg2 != but_old->arg2) {
		return false;
	}
	if (but_new->menu_create_func != but_old->menu_create_func || but_new->arg != but_old->arg) {
		return false;
	}
	if (but_new->pointer != but_old->pointer || but_new->pointype != but_old->pointype) {
		return false;
	}
	if (!STREQ(but_new->name, but_old->name)) {
		return false;
	}
	return true;
}

ROSE_INLINE uiBut *ui_but_find_old(uiBlock *block_old, const uiBut *but_new) {
	LISTBASE_FOREACH(uiBut *, but, &block_old->buttons) {
		if (ui_but_equals_old(but_new, but)) {
			return but;
		}
	}
	return NULL;
}

ROSE_INLINE void ui_but_update_old_active_from_new(uiBut *old, uiBut *but) {
	memcpy(&old->rect, &but->rect, sizeof(rctf));
	
	switch (old->type) {
		default: {
			/** Nothing to do here, this is button specific updates. */
		} break;
	}
}

ROSE_INLINE void ui_but_update_new_from_old_inactive(uiBut *but, uiBut *old) {
	/**
	 * Even though most of the times we are supposed to keep the button updated from 
	 * the pointer, we are keeping the old text for buttons that do not use pointer data.
	 * Thus allowing text gathering straight from the button buffers (even though it is not 
	 * recommended).
	 */
	if (!but->pointer) {
		SWAP(char *, but->drawstr, old->drawstr);
	}
}

ROSE_STATIC bool ui_but_update_from_old_block(struct rContext *C, uiBlock *block, uiBut **but_p, uiBut **old_p) {
	uiBlock *oldblock = block->oldblock;

	uiBut *but = *but_p;
	uiBut *old = *old_p;

	if (old == NULL || !ui_but_equals_old(but, old)) {
		old = ui_but_find_old(oldblock, but);
	}
	*old_p = (old) ? old->next : NULL;

	if (!old) {
		return false;
	}

	bool was_active = old->active;
	if (was_active) {
		LIB_remlink(&oldblock->buttons, old);
		LIB_insertlinkafter(&block->buttons, but, old);
		old->block = block;

		*but_p = old;

		ui_but_update_old_active_from_new(old, but);

		LIB_remlink(&block->buttons, but);
		ui_but_free(C, but);
	}
	else {
		LIB_remlink(&oldblock->buttons, old);
		
		ui_but_update_new_from_old_inactive(but, old);

		ui_but_free(C, old);
	}
	return was_active;
}

ROSE_STATIC void ui_but_mem_delete(uiBut *but) {
	MEM_SAFE_FREE(but->name);
	MEM_SAFE_FREE(but->drawstr);
	MEM_freeN(but);
}

ROSE_STATIC void ui_but_free(struct rContext *C, uiBut *but) {
	if (but->active) {
		if (C) {
			ui_but_active_free(C, but);
		}
		else {
			MEM_freeN(but->active);
		}
	}
	ui_but_mem_delete(but);
}

void ui_block_to_region_fl(const ARegion *region, const uiBlock *block, float *x, float *y) {
	const int getsizex = LIB_rcti_size_x(&region->winrct) + 1;
	const int getsizey = LIB_rcti_size_y(&region->winrct) + 1;

	float gx = *x;
	float gy = *y;

	*x = ((float)getsizex) * (0.5f + 0.5f * (gx * block->winmat[0][0] + gy * block->winmat[1][0] + block->winmat[3][0]));
	*y = ((float)getsizey) * (0.5f + 0.5f * (gx * block->winmat[0][1] + gy * block->winmat[1][1] + block->winmat[3][1]));
}

void ui_block_to_window_fl(const ARegion *region, const uiBlock *block, float *x, float *y) {
	ui_block_to_region_fl(region, block, x, y);
	*x += region->winrct.xmin;
	*y += region->winrct.ymin;
}

void ui_block_to_window_rctf(const ARegion *region, const uiBlock *block, rctf *dst, const rctf *src) {
	memcpy(dst, src, sizeof(rctf));
	ui_block_to_window_fl(region, block, &dst->xmin, &dst->ymin);
	ui_block_to_window_fl(region, block, &dst->xmax, &dst->ymax);
}

void ui_window_to_block_rctf(const ARegion *region, const uiBlock *block, rctf *dst, const rctf *src) {
	memcpy(dst, src, sizeof(rctf));
	ui_window_to_block_fl(region, block, &dst->xmin, &dst->ymin);
	ui_window_to_block_fl(region, block, &dst->xmax, &dst->ymax);
}

void ui_window_to_block_fl(const ARegion *region, const uiBlock *block, float *x, float *y) {
	const float getsizex = (float)LIB_rcti_size_x(&region->winrct) + 1;
	const float getsizey = (float)LIB_rcti_size_y(&region->winrct) + 1;
	const int sx = region->winrct.xmin;
	const int sy = region->winrct.ymin;

	const float a = 0.5f * getsizex * block->winmat[0][0];
	const float b = 0.5f * getsizex * block->winmat[1][0];
	const float c = 0.5f * getsizex * (1.0f + block->winmat[3][0]);

	const float d = 0.5f * getsizey * block->winmat[0][1];
	const float e = 0.5f * getsizey * block->winmat[1][1];
	const float f = 0.5f * getsizey * (1.0f + block->winmat[3][1]);

	const float px = *x - sx;
	const float py = *y - sy;

	*y = (a * (py - f) + d * (c - px)) / (a * e - d * b);
	*x = (px - b * (*y) - c) / a;
}

ROSE_INLINE void ui_but_to_pixelrect(rcti *rect, const ARegion *region, const uiBlock *block, const uiBut *but) {
	rctf rectf;
	rcti recti;

	ui_block_to_window_rctf(region, block, &rectf, (but) ? &but->rect : &block->rect);
	LIB_rcti_rctf_copy_round(&recti, &rectf);
	LIB_rcti_translate(&recti, -region->winrct.xmin, -region->winrct.ymin);
	memcpy(rect, &recti, sizeof(recti));
}

ROSE_INLINE uiBut *ui_def_but(uiBlock *block, int type, const char *name, int x, int y, int w, int h, void *pointer, int pointype, int maxlen) {
	ROSE_assert(w >= 0 && h >= 0 || (ELEM(type, UI_BTYPE_SEPR, UI_BTYPE_HSPR, UI_BTYPE_VSPR)));

	uiBut *but = MEM_callocN(sizeof(uiBut), "uiBut");

	but->name = LIB_strdupN(name);
	but->pointer = pointer;
	/** We only care about maxlength for string buffers. */
	if (pointer && but->pointype == UI_POINTER_STR) {
		but->maxlength = maxlen;
	}
	else {
		but->pointype = pointer ? pointype : UI_POINTER_NIL;
		but->maxlength = maxlen ? maxlen : 64;
	}

	switch (but->pointype) {
		case UI_POINTER_INT: {
			but->hardmin = INT_MIN;
			but->hardmax = INT_MAX;
		} break;
		case UI_POINTER_FLT: {
			but->hardmin = -FLT_MAX;
			but->hardmax = FLT_MAX;
		} break;
		case UI_POINTER_DBL: {
			but->hardmin = DBL_MIN;
			but->hardmax = DBL_MAX;
		} break;
		case UI_POINTER_UINT: {
			but->hardmin = 0;
			but->hardmax = UINT_MAX;
		} break;
		default: {
			/**
			 * Fallback limits for unsupported or unknown pointer types.
			 * These are used for text input validation (e.g. integer parsing) and scroll values.
			 * INT_MIN/INT_MAX are chosen because they safely cover all reasonable UI use cases.
			 * No display is known to exceed the pixel range of a 32-bit int.
			 */
			but->hardmin = INT_MIN;
			but->hardmax = INT_MAX;
		} break;
	}
	
	but->drawstr = MEM_mallocN(but->maxlength, "uiBut::DrawStr");
	LIB_strcpy(but->drawstr, but->maxlength, but->name);

	but->rect.xmin = x;
	but->rect.ymin = y;
	but->rect.xmax = but->rect.xmin + w;
	but->rect.ymax = but->rect.ymin + h;
	but->type = type;
	but->flag = 0;

	if (block->layout) {
		ui_layout_add_but(block->layout, but);
	}
	but->block = block;
	LIB_addtail(&block->buttons, but);

	return but;
}

uiBut *uiDefBut(uiBlock *block, int type, const char *name, int x, int y, int w, int h, void *pointer, int pointype, int maxlen, int draw) {
	uiBut *but = ui_def_but(block, type, name, x, y, w, h, pointer, pointype, maxlen);
	but->draw = draw;
	return but;
}

uiBut *uiDefButEx(uiBlock *block, int type, const char *name, int x, int y, int w, int h, void *pointer, int pointype, double softmin, double softmax, int maxlen, int draw) {
	uiBut *but = ui_def_but(block, type, name, x, y, w, h, pointer, pointype, maxlen);
	but->draw = draw;
	but->softmin = softmin;
	but->softmax = softmax;
	if (!ELEM(pointype, UI_POINTER_NIL)) {
		ui_but_set_value(but, ui_but_get_value(but));
	}
	return but;
}

void uiButEnableFlag(uiBut *but, int flag) {
	but->flag |= flag;
}
void uiButDisableFlag(uiBut *but, int flag) {
	but->flag &= flag;
}

void UI_but_func_text_set(uiBut *but, uiButHandleTextFunc func, double softmin, double softmax) {
	ROSE_assert(but->hardmin <= but->softmin && but->softmax <= but->hardmax);

	but->handle_text_func = func;
	but->softmin = ROSE_MAX(softmin, but->hardmin);
	but->softmax = ROSE_MIN(softmax, but->hardmax);
}

void UI_but_func_set(uiBut *but, uiButHandleFunc func, void *arg1, void *arg2) {
	but->handle_func = func;
	but->arg1 = arg1;
	but->arg2 = arg2;
}

void UI_but_menu_set(uiBut *but, uiBlockCreateFunc func, void *arg) {
	but->menu_create_func = func;
	but->arg = arg;
}

bool ui_region_contains_point_px(const ARegion *region, const int xy[2]) {
	if (!LIB_rcti_isect_pt_v(&region->winrct, xy)) {
		return false;
	}

	return true;
}

bool ui_but_contains_px(const uiBut *but, const ARegion *region, const int xy[2]) {
	uiBlock *block = but->block;
	if (!ui_region_contains_point_px(region, xy)) {
		return false;
	}

	float mx = xy[0], my = xy[1];
	ui_window_to_block_fl(region, block, &mx, &my);

	if (!ui_but_contains_pt(but, mx, my)) {
		return false;
	}

	return true;
}

bool ui_but_contains_pt(const uiBut *but, float mx, float my) {
	return LIB_rctf_isect_pt(&but->rect, mx, my);
}

bool ui_but_contains_rect(const uiBut *but, const rctf *rect) {
	return LIB_rctf_isect(&but->rect, rect, NULL);
}

void ui_but_update(uiBut *but) {
	if (!ui_but_is_editing(but)) {
		but->scroll = 0;
	}

	if (but->pointer) {
		but->drawstr = MEM_reallocN(but->drawstr, but->maxlength + 1);
		switch (but->pointype) {
			case UI_POINTER_BYTE:
				if (!ui_but_is_editing(but)) {
					LIB_strnformat(but->drawstr, but->maxlength, (but->draw & UI_BUT_HEX) ? "0x%02x" : "%d", *(unsigned char *)but->pointer);
				}
				else {
					char *itr = but->drawstr;
					*(unsigned char *)but->pointer = strtol(itr, &itr, 0);
				}
				break;
			case UI_POINTER_STR:
				if (!ui_but_is_editing(but)) {
					/** Update the draw sting from the pointer data. */
					LIB_strcpy(but->drawstr, but->maxlength, but->pointer);
				}
				else {
					/** Update the pointer data from the draw string. */
					LIB_strcpy(but->pointer, but->maxlength, but->drawstr);
				}
				break;
			case UI_POINTER_INT:
				if (!ui_but_is_editing(but)) {
					LIB_strnformat(but->drawstr, but->maxlength, (but->draw & UI_BUT_HEX) ? "0x%08x" : "%d", *(int *)but->pointer);
				}
				else {
					char *itr = but->drawstr;
					*(int *)but->pointer = strtol(itr, &itr, 0);
				}
				break;
			case UI_POINTER_UINT:
				if (!ui_but_is_editing(but)) {
					LIB_strnformat(but->drawstr, but->maxlength, (but->draw & UI_BUT_HEX) ? "0x%08x" : "%d", *(int *)but->pointer);
				}
				else {
					char *itr = but->drawstr;
					*(int *)but->pointer = strtoul(itr, &itr, 0);
				}
				break;
			case UI_POINTER_FLT:
				if (!ui_but_is_editing(but)) {
					LIB_strnformat(but->drawstr, but->maxlength, "%f", *(float *)but->pointer);
				}
				else {
					*(float *)but->pointer = atof(but->drawstr);
				}
				break;
			case UI_POINTER_DBL:
				if (!ui_but_is_editing(but)) {
					LIB_strnformat(but->drawstr, but->maxlength, "%lf", *(double *)but->pointer);
				}
				else {
					*(double *)but->pointer = atof(but->drawstr);
				}
				break;
		}
	}
}

uiBut *ui_block_active_but_get(const uiBlock *block) {
	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		if (but->active) {
			return but;
		}
	}
	return NULL;
}

uiBut *ui_region_find_active_but(const ARegion *region) {
	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		uiBut *but = ui_block_active_but_get(block);
		if (but) {
			return but;
		}
	}
	return NULL;
}

uiBut *ui_but_find_mouse_over_ex(const ARegion *region, const int xy[2]) {
	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		float x = xy[0], y = xy[1];
		ui_window_to_block_fl(region, block, &x, &y);
		LISTBASE_FOREACH_BACKWARD(uiBut *, but, &block->buttons) {
			if (but->flag & UI_DISABLED || ELEM(but->type, UI_BTYPE_SEPR)) {
				continue;
			}
			if (ui_but_contains_pt(but, x, y)) {
				return but;
			}
		}
	}
	return NULL;
}

double ui_but_get_value(struct uiBut *but) {
	if (but->pointer == NULL) {
		return 0;
	}

	char *end = NULL;
	const int base = but->flag & UI_BUT_HEX ? 16 : 0;

	switch (but->pointype) {
		case UI_POINTER_BYTE:
			return (but->drawstr && ui_but_is_editing(but)) ? (double)strtol(but->drawstr, &end, base) : (double)*(const unsigned char *)but->pointer;
		case UI_POINTER_INT:
			return (but->drawstr && ui_but_is_editing(but)) ? (double)strtol(but->drawstr, &end, base) : (double)*(const int *)but->pointer;
		case UI_POINTER_FLT:
			return (but->drawstr && ui_but_is_editing(but)) ? (double)strtof(but->drawstr, &end) : (double)*(const float *)but->pointer;
		case UI_POINTER_DBL:
			return (but->drawstr && ui_but_is_editing(but)) ? (double)strtod(but->drawstr, &end) : (double)*(const double *)but->pointer;
		case UI_POINTER_UINT:
			return (but->drawstr && ui_but_is_editing(but)) ? (double)strtoul(but->drawstr, &end, base) : (double)*(const unsigned int *)but->pointer;
	}

	return 0;
}

double ui_but_set_value(struct uiBut *but, double nvalue) {
	ROSE_assert(but->hardmin <= but->softmin && but->softmax <= but->hardmax);

	nvalue = ROSE_MAX(but->softmin, ROSE_MIN(nvalue, but->softmax));

	switch (but->pointype) {
		case UI_POINTER_BYTE: {
			*(unsigned char *)but->pointer = (unsigned char)round(nvalue);
		} break;
		case UI_POINTER_INT: {
			*(int *)but->pointer = (int)round(nvalue);
		} break;
		case UI_POINTER_FLT: {
			*(float *)but->pointer = (float)nvalue;
		} break;
		case UI_POINTER_DBL: {
			*(double *)but->pointer = (double)nvalue;
		} break;
		case UI_POINTER_UINT: {
			*(unsigned int *)but->pointer = (unsigned int)round(nvalue);
		} break;
		default:
			ROSE_assert_unreachable();
	}

	return nvalue;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Builtin UI Functions
 * \{ */

bool uiButHandleTextFunc_Integer(struct rContext *C, uiBut *but, const char *edit) {
	char *end;
	if (but->hardmin < 0) {
		long long value = strtoll(edit, &end, (but->flag & UI_BUT_HEX) ? 16 : 0);
		while (isspace((unsigned char)*end)) {
			end++;
		}
		return ELEM(*end, '\0') && (value >= (long long)but->softmin && value <= (long long)but->softmax);
	}
	else {
		unsigned long long value = strtoull(edit, &end, (but->flag & UI_BUT_HEX) ? 16 : 0);
		while (isspace((unsigned char)*end)) {
			end++;
		}
		return ELEM(*end, '\0') && (value >= (unsigned long long)but->softmin && value <= (unsigned long long)but->softmax);
	}
	return false;
}

bool uiButHandleTextFunc_Decimal(struct rContext *C, uiBut *but, const char *edit) {
	char *end;
	long double value = strtold(edit, &end);
	while (isspace((unsigned char)*end)) {
		end++;
	}
	return ELEM(*end, '\0') && (value >= (double)but->softmin && value <= (double)but->softmax);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Block
 * \{ */

ROSE_INLINE void ui_update_window_matrix(wmWindow *window, ARegion *region, uiBlock *block) {
	if (region && region->visible) {
		GPU_matrix_projection_get(block->winmat);
	}
	else {
		const int sizex = WM_window_size_x(window);
		const int sizey = WM_window_size_y(window);
		const rcti winrct = {
			.xmin = 0,
			.xmax = sizex - 1,
			.ymin = 0,
			.ymax = sizey - 1,
		};
		WM_get_projection_matrix(block->winmat, &winrct);
	}
}

uiBlock *UI_block_begin(struct rContext *C, ARegion *region, const char *name) {
	wmWindow *window = CTX_wm_window(C);

	uiBlock *block = MEM_callocN(sizeof(uiBlock), "uiBlock");
	block->oldblock = NULL;

	block->name = LIB_strdupN(name);
	block->active = true;

	block->layout = NULL;
	LIB_listbase_clear(&block->buttons);
	LIB_listbase_clear(&block->layouts);

	if (region) {
		UI_block_region_set(block, region);
	}

	ui_update_window_matrix(window, region, block);
	return block;
}

void UI_block_update_from_old(struct rContext *C, uiBlock *block) {
	if (!block->oldblock) {
		return;
	}

	uiBut *but_old = (uiBut *)block->oldblock->buttons.first;

	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		if (ui_but_update_from_old_block(C, block, &but, &but_old)) {
			ui_but_update(but);
		}
	}

	block->oldblock = NULL;
}

ROSE_INLINE void ui_block_bounds_calc(uiBlock *block) {
	if (!LIB_listbase_is_empty(&block->buttons)) {
		LIB_rctf_init_minmax(&block->rect);

		LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
			LIB_rctf_union(&block->rect, &but->rect);
		}

		block->rect.xmin -= block->bounds;
		block->rect.ymin -= block->bounds;
		block->rect.xmax += block->bounds;
		block->rect.ymax += block->bounds;
	}
	block->rect.xmax = block->rect.xmin + ROSE_MAX(LIB_rctf_size_x(&block->rect), block->minbounds);
}

void UI_block_end_ex(struct rContext *C, uiBlock *block, const int xy[2], int r_xy[2]) {
	wmWindow *window = CTX_wm_window(C);
	ARegion *region = CTX_wm_region(C);

	ROSE_assert(block->active);

	if (block->layouts.first) {
		UI_block_layout_resolve(block, NULL, NULL);
	}
	UI_block_update_from_old(C, block);
	ui_block_bounds_calc(block);
	block->closed = true;
}

void UI_block_end(struct rContext *C, uiBlock *block) {
	wmWindow *window = CTX_wm_window(C);

	UI_block_end_ex(C, block, window->event_state->mouse_xy, NULL);
}

ROSE_INLINE void ui_draw_menu_back(ARegion *region, uiBlock *block, const rcti *rect) {
}

void UI_block_draw(const struct rContext *C, uiBlock *block) {
	ARegion *region = CTX_wm_region(C);

	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		/** Before drawing we update the buttons so that all the content is curernt. */
		ui_but_update(but);
	}

	GPU_blend(GPU_BLEND_ALPHA);

	GPU_matrix_push_projection();
	GPU_matrix_push();
	GPU_matrix_identity_set();

	RFT_batch_draw_begin();

	rcti rect;
	ui_but_to_pixelrect(&rect, region, block, NULL);

	ui_draw_menu_back(region, block, &rect);

	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		ui_but_to_pixelrect(&rect, region, block, but);
		if (rect.xmin < rect.xmax && rect.ymin < rect.ymax) {
			ui_draw_but(C, region, but, &rect);
		}
	}

	RFT_batch_draw_end();

	GPU_matrix_pop();
	GPU_matrix_pop_projection();
}

void UI_block_region_set(uiBlock *block, ARegion *region) {
	ListBase *lb = &region->uiblocks;
	uiBlock *oldblock = NULL;
	if (lb) {
		if (region->runtime.block_name_map == NULL) {
			region->runtime.block_name_map = LIB_ghash_str_new(__func__);
		}
		oldblock = (uiBlock *)LIB_ghash_lookup(region->runtime.block_name_map, block->name);

		if (oldblock) {
			oldblock->active = false;
		}

		LIB_addhead(lb, block);
		LIB_ghash_reinsert(region->runtime.block_name_map, block->name, block, NULL, NULL);
	}
	block->oldblock = oldblock;
}

void UI_block_translate(uiBlock *block, float dx, float dy) {
	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		LIB_rctf_translate(&but->rect, dx, dy);
	}
	LIB_rctf_translate(&block->rect, dx, dy);
}

void UI_block_scroll(ARegion *region, uiBlock *block, uiLayout *layout) {
	uiBut *but;

	int w, h;
	UI_layout_estimate(layout, &w, &h);
	UI_block_layout_set_current(block, NULL);

	int x = region->sizex - V2D_SCROLL_WIDTH + 1;
	double pages = ROSE_MAX(((float)h / (float)region->sizey), 1);

	if (((float)h / (float)region->sizey) > 1) {
		but = uiDefButEx(block, UI_BTYPE_SCROLL, "", x, 0, V2D_SCROLL_WIDTH, region->sizey, &region->vscroll, UI_POINTER_FLT, 1, pages, 0, 0);
		uiButEnableFlag(but, UI_DEFAULT);
	}
	else {
		region->vscroll = 1;
	}
}

void UI_blocklist_update_window_matrix(struct rContext *C, ARegion *region) {
	wmWindow *window = CTX_wm_window(C);

	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		if (block->active) {
			ui_update_window_matrix(window, region, block);
		}
	}
}

void UI_blocklist_free(struct rContext *C, ARegion *region) {
	ListBase *lb = &region->uiblocks;
	for (uiBlock *block; block = (uiBlock *)LIB_pophead(lb);) {
		UI_block_free(C, block);
	}
	if (region->runtime.block_name_map != NULL) {
		LIB_ghash_free(region->runtime.block_name_map, NULL, NULL);
		region->runtime.block_name_map = NULL;
	}
}

void UI_region_free_active_but_all(struct rContext *C, ARegion *region) {
	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
			if (but->active == NULL) {
				continue;
			}
			ui_but_active_free(C, but);
		}
	}
}

void UI_blocklist_free_inactive(struct rContext *C, ARegion *region) {
	ListBase *lb = &region->uiblocks;

	LISTBASE_FOREACH_MUTABLE(uiBlock *, block, lb) {
		if (block->active) {
			block->active = false;
		}
		else {
			if (region->runtime.block_name_map != NULL) {
				uiBlock *b = (uiBlock *)LIB_ghash_lookup(region->runtime.block_name_map, block->name);
				if (b == block) {
					LIB_ghash_remove(region->runtime.block_name_map, block->name, NULL, NULL);
				}
			}
			LIB_remlink(lb, block);
			UI_block_free(C, block);
		}
	}
}

void UI_block_free(struct rContext *C, uiBlock *block) {
	for (uiBut *but; but = (uiBut *)LIB_pophead(&block->buttons);) {
		ui_but_free(C, but);
	}

	MEM_freeN(block->name);
	MEM_freeN(block);
}

/** \} */
