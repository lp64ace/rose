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
#include "KER_screen.h"

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

ROSE_STATIC void ui_but_free(rContext *C, uiBut *but);

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
	if (but_new->rna_pointer.data != but_old->rna_pointer.data || but_new->rna_property != but_old->rna_property || but_new->rna_index != but_old->rna_index) {
		return false;
	}
	if (but_new->poin != but_old->poin || but_new->pointype != but_old->pointype) {
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
	if (!but->poin) {
		SWAP(char *, but->drawstr, old->drawstr);
	}
}

ROSE_STATIC bool ui_but_update_from_old_block(rContext *C, uiBlock *block, uiBut **but_p, uiBut **old_p) {
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

ROSE_STATIC void ui_but_free(rContext *C, uiBut *but) {
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

	if (block->panel) {
		gx += block->panel->ofsx;
		gy += block->panel->ofsy;
	}

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

	if (block->panel) {
		*x -= block->panel->ofsx;
		*y -= block->panel->ofsy;
	}
}

ROSE_INLINE void ui_but_to_pixelrect(rcti *rect, const ARegion *region, const uiBlock *block, const uiBut *but) {
	rctf rectf;
	rcti recti;

	ui_block_to_window_rctf(region, block, &rectf, (but) ? &but->rect : &block->rect);
	LIB_rcti_rctf_copy_round(&recti, &rectf);
	LIB_rcti_translate(&recti, -region->winrct.xmin, -region->winrct.ymin);
	memcpy(rect, &recti, sizeof(recti));
}

ROSE_INLINE uiBut *ui_def_but(uiBlock *block, int type, const char *name, int x, int y, int w, int h, void *poin, int pointype, float min, float max) {
	uiBut *but = (uiBut *)MEM_callocN(sizeof(uiBut), "uiBut");

	but->name = LIB_strdupN(name);

	switch (pointype) {
		case UI_POINTER_STR: {
			but->maxlength = (int)(max) ? (int)(max) : 64;
		} break;
		default: {
			but->maxlength = 16;
		} break;
	}

	but->drawstr = MEM_mallocN(but->maxlength, "uiBut::DrawStr");
	LIB_strcpy(but->drawstr, but->maxlength, but->name);

	but->pointype = pointype;
	but->poin = poin;
	but->hardmin = but->softmin = min;
	but->hardmax = but->softmax = max;
	but->precision = -1.0;

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

ROSE_INLINE uiBut *ui_def_but_rna(uiBlock *block, int type, const char *name, int x, int y, int w, int h, PointerRNA *pointer, PropertyRNA *property, int index) {
	ROSE_assert(w >= 0 && h >= 0 || (ELEM(type, UI_BTYPE_SEPR, UI_BTYPE_HSPR, UI_BTYPE_VSPR)));

	float min = 0;
	float max = 0;

	float precision = -1.0f;
	switch (RNA_property_type(property)) {
		case PROP_INT: {
			int hardmin, hardmax, softmin, softmax, step;

			RNA_property_int_range(pointer, property, &hardmin, &hardmax);
			RNA_property_int_ui_range(pointer, property, &softmin, &softmax, &step);
		} break;
		case PROP_FLOAT: {
			float hardmin, hardmax, softmin, softmax, step;

			RNA_property_float_range(pointer, property, &hardmin, &hardmax);
			RNA_property_float_ui_range(pointer, property, &softmin, &softmax, &step, &precision);
		} break;
		case PROP_STRING: {
			max = RNA_property_string_max_length(property);
		} break;
	}

	uiBut *but = ui_def_but(block, type, name, x, y, w, h, NULL, UI_POINTER_NIL, min, max);

	if (pointer) {
		but->rna_pointer = *pointer;
		but->rna_property = property;

		if (RNA_property_array_check(but->rna_property)) {
			but->rna_index = index;
		}
		else {
			but->rna_index = 0;
		}

		but->precision = precision;
	}
	else {
		but->rna_pointer = PointerRNA_NULL;
	}

	return but;
}

uiBut *uiDefBut(uiBlock *block, int type, const char *name, int x, int y, int w, int h, void *poin, int pointype, float min, float max, int draw) {
	uiBut *but = ui_def_but(block, type, name, x, y, w, h, poin, pointype, min, max);
	if (but) {
		but->draw = draw;
	}
	return but;
}

uiBut *uiDefBut_RNA(uiBlock *block, int type, const char *name, int x, int y, int w, int h, struct PointerRNA *pointer, const char *property, int index, int draw) {
	PathResolvedRNA rna = {
		.index = index,
	};

	if (RNA_path_resolve_property(pointer, property, &rna.ptr, &rna.property)) {
		return uiDefButEx_RNA(block, type, name, x, y, w, h, &rna.ptr, rna.property, rna.index, draw);
	}
	return NULL;
}

uiBut *uiDefButEx_RNA(uiBlock *block, int type, const char *name, int x, int y, int w, int h, struct PointerRNA *pointer, struct PropertyRNA *property, int index, int draw) {
	uiBut *but = ui_def_but_rna(block, type, name, x, y, w, h, pointer, property, index);
	if (but) {
		but->draw = draw;
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

/** \} */

/* -------------------------------------------------------------------- */
/** \name Builtin UI Functions
 * \{ */

bool uiButHandleTextFunc_Integer(rContext *C, uiBut *but, const char *edit) {
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

bool uiButHandleTextFunc_Decimal(rContext *C, uiBut *but, const char *edit) {
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

uiBlock *UI_block_begin(rContext *C, ARegion *region, const char *name) {
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

void UI_block_update_from_old(rContext *C, uiBlock *block) {
	if (!block->oldblock) {
		return;
	}

	uiBut *but_old = (uiBut *)block->oldblock->buttons.first;

	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		if (ui_but_update_from_old_block(C, block, &but, &but_old)) {
			ui_but_update(but, false);
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


ROSE_INLINE void ui_offset_panel_block(uiBlock *block) {
	/* Compute bounds and offset. */
	ui_block_bounds_calc(block);

	const int ofsy = block->panel->sizey;

	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		but->rect.ymin += ofsy;
		but->rect.ymax += ofsy;
	}

	block->rect.xmax = block->panel->sizex;
	block->rect.ymax = block->panel->sizey;
	block->rect.xmin = block->rect.ymin = 0.0;
}

void UI_block_end_ex(rContext *C, uiBlock *block, const int xy[2], int r_xy[2]) {
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

void UI_block_end(rContext *C, uiBlock *block) {
	wmWindow *window = CTX_wm_window(C);

	UI_block_end_ex(C, block, window->event_state->mouse_xy, NULL);
}

ROSE_INLINE void ui_draw_menu_back(ARegion *region, uiBlock *block, const rcti *rect) {
}

void UI_block_draw(const rContext *C, uiBlock *block) {
	ARegion *region = CTX_wm_region(C);

	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		/** Before drawing we update the buttons so that all the content is curernt. */
		ui_but_update(but, false);
	}

	GPU_blend(GPU_BLEND_ALPHA);

	GPU_matrix_push_projection();
	GPU_matrix_push();
	GPU_matrix_identity_set();

	ED_region_pixelspace(region);

	RFT_batch_draw_begin();

	rcti rect;
	ui_but_to_pixelrect(&rect, region, block, NULL);

	ui_draw_menu_back(region, block, &rect);

	// fprintf(stdout, "[Editor] Drawing uiBlock \"%s\" [xmin: %d, xmax: %d, ymin: %d, ymax: %d]\n", block->name, rect.xmin, rect.xmax, rect.ymin, rect.ymax);
	// fprintf(stdout, "ARegion [xmin: %d, xmax: %d, ymin: %d, ymax: %d]\n", region->winrct.xmin, region->winrct.xmax, region->winrct.ymin, region->winrct.ymax);
	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		ui_but_to_pixelrect(&rect, region, block, but);
		if (rect.xmin < rect.xmax && rect.ymin < rect.ymax) {
			// fprintf(stdout, "\tuiBut : \"%s\" [xmin: %d, xmax: %d, ymin: %d, ymax: %d]\n", but->name, rect.xmin, rect.xmax, rect.ymin, rect.ymax);
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

void UI_blocklist_update_window_matrix(rContext *C, ARegion *region) {
	wmWindow *window = CTX_wm_window(C);

	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		if (block->active) {
			ui_update_window_matrix(window, region, block);
		}
	}
}

void UI_blocklist_free(rContext *C, ARegion *region) {
	ListBase *lb = &region->uiblocks;
	for (uiBlock *block; block = (uiBlock *)LIB_pophead(lb);) {
		UI_block_free(C, block);
	}
	if (region->runtime.block_name_map != NULL) {
		LIB_ghash_free(region->runtime.block_name_map, NULL, NULL);
		region->runtime.block_name_map = NULL;
	}
}

void UI_region_free_active_but_all(rContext *C, ARegion *region) {
	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
			if (but->active == NULL) {
				continue;
			}
			ui_but_active_free(C, but);
		}
	}
}

void UI_blocklist_free_inactive(rContext *C, ARegion *region) {
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

void UI_block_free(rContext *C, uiBlock *block) {
	for (uiBut *but; but = (uiBut *)LIB_pophead(&block->buttons);) {
		ui_but_free(C, but);
	}

	MEM_freeN(block->name);
	MEM_freeN(block);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Panel
 * \{ */

static void panels_layout_begin_clear_flags(ListBase *lb) {
	LISTBASE_FOREACH(Panel *, panel, lb) {
		/* Flags to copy over to the next layout pass. */
		const short flag_copy = 0;

		const bool was_active = panel->flag_ex & PANEL_ACTIVE;
		panel->flag_ex &= flag_copy;

		panels_layout_begin_clear_flags(&panel->children);
	}
}

static int get_panel_size_y(const Panel *panel) {
	if (panel->type && (panel->type->flag & PANEL_TYPE_NO_HEADER)) {
		return panel->sizey;
	}

	return UI_UNIT_Y + panel->sizey;
}

static int get_panel_real_size_y(const Panel *panel) {
	const int sizey = panel->sizey;

	if (panel->type && (panel->type->flag & PANEL_TYPE_NO_HEADER)) {
		return sizey;
	}

	return UI_UNIT_Y + sizey;
}

static float panel_region_offset_x_get(const ARegion *region) {
	if (RGN_ALIGN_ENUM_FROM_MASK(region->alignment) != RGN_ALIGN_RIGHT) {
		return UI_UNIT_X;
	}

	return 0.0f;
}

/**
 * Starting from the "block size" set in #UI_panel_end, calculate the full size
 * of the panel including the sub-panel headers and buttons.
 */
static void panel_calculate_size_recursive(ARegion *region, Panel *panel) {
	int width = panel->blocksizex;
	int height = panel->blocksizey;

	LISTBASE_FOREACH(Panel *, child_panel, &panel->children) {
		if (child_panel->flag_ex & PANEL_ACTIVE) {
			panel_calculate_size_recursive(region, child_panel);
			width = ROSE_MAX(width, child_panel->sizex);
			height += get_panel_real_size_y(child_panel);
		}
	}

	/* Update total panel size. */
	if (panel->flag_ex & PANEL_NEW_ADDED) {
		panel->flag_ex &= ~PANEL_NEW_ADDED;
		panel->sizex = width;
		panel->sizey = height;
	}
	else {
		const int old_sizex = panel->sizex, old_sizey = panel->sizey;
		const int old_region_ofsx = panel->runtime.region_ofsx;

		/* Update width/height if non-zero. */
		if (width != 0) {
			panel->sizex = width;
		}
		if (height != 0) {
			panel->sizey = height;
		}

		if (panel->sizex != old_sizex || panel->sizey != old_sizey) {
			panel->ofsy += old_sizey - panel->sizey;
		}

		panel->runtime.region_ofsx = panel_region_offset_x_get(region);
	}
}

void UI_panels_begin(const rContext *C, ARegion *region) {
	panels_layout_begin_clear_flags(&region->panels);
}

/**
 * This function is needed because #uiBlock and Panel itself don't
 * change #Panel.sizey or location when closed.
 */
static int get_panel_real_ofsy(Panel *panel) {
	return panel->ofsy + panel->sizey;
}

bool UI_panel_should_show_background(const ARegion *region, const PanelType *panel_type) {
	if (region->alignment == RGN_ALIGN_FLOAT) {
		return false;
	}

	if (panel_type && panel_type->flag & PANEL_TYPE_NO_HEADER) {
		if (region->regiontype == RGN_TYPE_TOOLS) {
			/* We never want a background around active tools. */
			return false;
		}
		/* Without a header there is no background except for region overlap. */
		return region->overlap != 0;
	}

	return true;
}

static void ui_panels_size(ARegion *region, int *r_x, int *r_y) {
	int sizex = 0;
	int sizey = 0;
	bool has_panel_with_background = false;

	/* Compute size taken up by panels, for setting in view2d. */
	LISTBASE_FOREACH(Panel *, panel, &region->panels) {
		if (panel->flag_ex & PANEL_ACTIVE) {
			const int pa_sizex = panel->ofsx + panel->sizex;
			const int pa_sizey = get_panel_real_ofsy(panel);

			sizex = ROSE_MAX(sizex, pa_sizex);
			sizey = ROSE_MIN(sizey, pa_sizey);
			if (UI_panel_should_show_background(region, panel->type)) {
				has_panel_with_background = true;
			}
		}
	}

	if (sizex == 0) {
		sizex = (15 * UI_UNIT_X);
	}
	if (sizey == 0) {
		sizey = -(15 * UI_UNIT_X);
	}
	/* Extra margin after the list so the view scrolls a few pixels further than the panel border.
	 * Also makes the bottom match the top margin. */
	if (has_panel_with_background) {
		sizey -= UI_UNIT_Y;
	}

	*r_x = sizex;
	*r_y = sizey;
}

void UI_panels_end(const rContext *C, ARegion *region, int *r_x, int *r_y) {
	ScrArea *area = CTX_wm_area(C);

	LISTBASE_FOREACH(Panel *, panel, &region->panels) {
		if (panel->flag_ex & PANEL_ACTIVE) {
			ROSE_assert(panel->runtime.block != NULL);
			panel_calculate_size_recursive(region, panel);
		}
	}

	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		if (block->panel) {
			ui_offset_panel_block(block);
		}
	}

	/* Compute size taken up by panels. */
	ui_panels_size(region, r_x, r_y);
}

void UI_panels_draw(const rContext *C, ARegion *region) {
	/* Draw in reverse order, because #uiBlocks are added in reverse order
	 * and we need child panels to draw on top. */
	LISTBASE_FOREACH_BACKWARD(uiBlock *, block, &region->uiblocks) {
		if (block->panel) {
			UI_block_draw(C, block);
		}
	}
}

void UI_panel_drawname_set(Panel *panel, const char *drawname) {
	MEM_SAFE_FREE(panel->drawname);
	panel->drawname = LIB_strdupN(drawname);
}

Panel *UI_panel_begin(ARegion *region, ListBase *lb, uiBlock *block, PanelType *pt, Panel *panel) {
	Panel *panel_last;
	const char *drawname = pt->label;
	const bool newpanel = (panel == NULL);

	if (newpanel) {
		panel = KER_panel_new(pt);

		panel->ofsx = 0;
		panel->ofsy = 0;
		panel->sizex = 0;
		panel->sizey = 0;
		panel->blocksizex = 0;
		panel->blocksizey = 0;
		panel->flag_ex |= PANEL_NEW_ADDED;

		LIB_addtail(lb, panel);
	}
	else {
		/* Panel already exists. */
		panel->type = pt;
	}

	panel->runtime.block = block;

	UI_panel_drawname_set(panel, drawname);

	/* If a new panel is added, we insert it right after the panel that was last added.
	 * This way new panels are inserted in the right place between versions. */
	for (panel_last = (Panel *)(lb->first); panel_last; panel_last = panel_last->next) {
		if (panel_last->flag_ex & PANEL_LAST_ADDED) {
			LIB_remlink(lb, panel);
			LIB_insertlinkafter(lb, panel_last, panel);
			break;
		}
	}

	if (newpanel) {
		panel->sortorder = (panel_last) ? panel_last->sortorder + 1 : 0;

		LISTBASE_FOREACH(Panel *, panel_next, lb) {
			if (panel_next != panel && panel_next->sortorder >= panel->sortorder) {
				panel_next->sortorder++;
			}
		}
	}

	if (panel_last) {
		panel_last->flag_ex &= ~PANEL_LAST_ADDED;
	}

	/* Assign the new panel to the block. */
	block->panel = panel;
	panel->flag_ex |= PANEL_ACTIVE | PANEL_LAST_ADDED;

	return panel;
}

void UI_panel_header_buttons_begin(Panel *panel) {
	uiBlock *block = panel->runtime.block;

	// ui_block_new_button_group(block, UI_BUTTON_GROUP_LOCK | UI_BUTTON_GROUP_PANEL_HEADER);
}

void UI_panel_header_buttons_end(Panel *panel) {
	uiBlock *block = panel->runtime.block;

	/* Always add a new button group. Although this may result in many empty groups, without it,
	 * new buttons in the panel body not protected with a #ui_block_new_button_group call would
	 * end up in the panel header group. */
	// ui_block_new_button_group(block, 0);
}

void UI_panel_end(Panel *panel, int width, int height) {
	/* Store the size of the buttons layout in the panel. The actual panel size
	 * (including sub-panels) is calculated in #UI_panels_end. */
	panel->blocksizex = width;
	panel->blocksizey = height;
}

Panel *UI_panel_find_by_type(ListBase *lb, const PanelType *pt) {
	const char *idname = pt->idname;

	LISTBASE_FOREACH(Panel *, panel, lb) {
		if (STREQLEN(panel->name, idname, sizeof(panel->name))) {
			return panel;
		}
	}
	return NULL;
}

void UI_panel_label_offset(const uiBlock *block, int *r_x, int *r_y) {
	Panel *panel = block->panel;
	const bool is_subpanel = (panel->type && panel->type->parent);

	*r_x = UI_UNIT_X * 1.0f;
	*r_y = UI_UNIT_Y * 1.5f;

	if (is_subpanel) {
		*r_x += (0.7f * UI_UNIT_X);
	}
}

/** \} */
