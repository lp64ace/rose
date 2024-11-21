#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"
#include "UI_interface.h"

#include "KER_context.h"

#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "LIB_listbase.h"
#include "LIB_ghash.h"
#include "LIB_string.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "RFT_api.h"

#include "interface_intern.h"

/* -------------------------------------------------------------------- */
/** \name UI Button
 * \{ */

ROSE_STATIC void ui_but_free(const struct rContext *C, uiBut *but);

ROSE_INLINE bool ui_but_equals_old(const uiBut *but_new, const uiBut *but_old) {
	if(but_new->type != but_old->type) {
		return false;
	}
	if(!STREQ(but_new->name, but_old->name)) {
		return false;
	}
	return true;
}

ROSE_INLINE uiBut *ui_but_find_old(uiBlock *block_old, const uiBut *but_new) {
	LISTBASE_FOREACH(uiBut *, but, &block_old->buttons) {
		if(ui_but_equals_old(but_new, but)) {
			return but;
		}
	}
	return NULL;
}

ROSE_INLINE void ui_but_update_old_active_from_new(uiBut *old, uiBut *but) {
	ROSE_assert(old->active);
	
	memcpy(&old->rect, &but->rect, sizeof(rctf));
	
	switch(old->type) {
		default: {
			/** Nothing to do here, this is button specific updates. */
		} break;
	}
}

ROSE_STATIC bool ui_but_update_from_old_block(const struct rContext *C, uiBlock *block, uiBut **but_p, uiBut **old_p) {
	uiBlock *oldblock = block->oldblock;
	
	uiBut *but = *but_p;
	uiBut *old = *old_p;
	
	if(old == NULL || !ui_but_equals_old(but, old)) {
		old = ui_but_find_old(oldblock, but);
	}
	*old_p = (old) ? old->next : NULL;
	
	if(!old) {
		return false;
	}
	
	bool was_active = old->active;
	if(old->active) {
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
		ui_but_free(C, old);
	}
	return was_active;
}

ROSE_STATIC void ui_but_mem_delete(uiBut *but) {
	MEM_freeN(but->name);
	MEM_freeN(but);
}

ROSE_STATIC void ui_but_free(const struct rContext *C, uiBut *but) {
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

ROSE_INLINE void ui_block_to_window_fl(const ARegion *region, const uiBlock *block, float *x, float *y) {
	ui_block_to_region_fl(region, block, x, y);
	*x += region->winrct.xmin;
	*y += region->winrct.ymin;
}

ROSE_INLINE void ui_block_to_window_rctf(const ARegion *region, const uiBlock *block, rctf *dst, const rctf *src) {
	memcpy(dst, src, sizeof(rctf));
	ui_block_to_window_fl(region, block, &dst->xmin, &dst->ymin);
	ui_block_to_window_fl(region, block, &dst->xmax, &dst->ymax);
}

ROSE_INLINE void ui_but_to_pixelrect(rcti *rect, const ARegion *region, const uiBlock *block, const uiBut *but) {
	rctf rectf;
	rcti recti;

	ui_block_to_window_rctf(region, block, &rectf, (but) ? &but->rect : &block->rect);
	LIB_rcti_rctf_copy_round(&recti, &rectf);
	LIB_rcti_translate(&recti, -region->winrct.xmin, -region->winrct.ymin);
	memcpy(rect, &recti, sizeof(recti));
}

uiBut *uiDefBut(uiBlock *block, int type, const char *name, int x, int y, int w, int h) {
	ROSE_assert(w >= 0 && h >= 0 || (type == UI_BTYPE_SEPR));
	
	uiBut *but = MEM_callocN(sizeof(uiBut), "uiBut");
	
	but->name = LIB_strdupN(name);
	but->active = true;
	but->rect.xmin = x;
	but->rect.ymin = y;
	but->rect.xmax = but->rect.xmin + w;
	but->rect.ymax = but->rect.ymin + h;
	but->type = type;
	but->flag = 0;
	
	if(block->layout) {
		ui_layout_add_but(block->layout, but);
	}
	but->block = block;
	
	LIB_addtail(&block->buttons, but);
	
	return but;
}

void ui_but_update(uiBut *but) {
}

ROSE_INLINE void ui_draw_text(uiBut *but, const rcti *rect) {
	int font = RFT_set_default();
	RFT_clipping(font, rect->xmin, rect->ymin, rect->xmax, rect->ymax);
	RFT_color3f(font, 0.0f, 0.0f, 0.0f);
	
	int cx = LIB_rcti_cent_x(rect);
	int cy = LIB_rcti_cent_y(rect);

	rcti bound;
	RFT_boundbox(font, but->name, -1, &bound);
	/** By default text is horizontally centered. */
	RFT_position(font, cx - LIB_rcti_size_x(&bound) / 2, cy - RFT_height_max(font) / 3, -1.0f);
	RFT_draw(font, but->name, -1);
}

ROSE_STATIC void ui_draw_but(const struct rContext *C, ARegion *region, uiBut *but, const rcti *rect) {
	if (but->type == UI_BTYPE_SEPR) {
		return;
	}

	rctf rectf;
	LIB_rctf_rcti_copy(&rectf, rect);

	UI_draw_roundbox_4fv(&rectf, true, 0.0f, (const float[4]){0.5f, 0.5f, 0.5f, 1.0f});

	ui_draw_text(but, rect);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name UI Block
 * \{ */

ROSE_INLINE void ui_update_window_matrix(wmWindow *window, ARegion *region, uiBlock *block) {
	if(region && region->visible) {
		GPU_matrix_projection_get(block->winmat);
	}
	else {
		const int sizex = WM_window_width(window);
		const int sizey = WM_window_height(window);
		const rcti winrct = {
			.xmin = 0,
			.xmax = sizex - 1,
			.ymin = 0,
			.ymax = sizey - 1,
		};
		WM_get_projection_matrix(block->winmat, &winrct);
	}
}

uiBlock *UI_block_begin(const struct rContext *C, ARegion *region, const char *name) {
	wmWindow *window = CTX_wm_window(C);
	
	uiBlock *block = MEM_callocN(sizeof(uiBlock), "uiBlock");
	block->oldblock = NULL;
	
	block->name = LIB_strdupN(name);
	block->active = true;

	block->layout = NULL;
	LIB_listbase_clear(&block->buttons);
	LIB_listbase_clear(&block->layouts);
	
	if(region) {
		UI_block_region_set(block, region);
	}
	
	ui_update_window_matrix(window, region, block);
	return block;
}

void UI_block_update_from_old(const struct rContext *C, uiBlock *block) {
	if(!block->oldblock) {
		return;
	}
	
	uiBut *but_old = (uiBut *)block->oldblock->buttons.first;
	
	LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
		if(ui_but_update_from_old_block(C, block, &but, &but_old)) {
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

void UI_block_end_ex(const struct rContext *C, uiBlock *block, const int xy[2], int r_xy[2]) {
	wmWindow *window = CTX_wm_window(C);
	ARegion *region = CTX_wm_region(C);
	
	ROSE_assert(block->active);
	
	if(block->layouts.first) {
		UI_block_layout_resolve(block, NULL, NULL);
	}
	UI_block_update_from_old(C, block);
	ui_block_bounds_calc(block);
}

void UI_block_end(const struct rContext *C, uiBlock *block) {
	wmWindow *window = CTX_wm_window(C);
	
	UI_block_end_ex(C, block, window->event_state->mouse_xy, NULL);
}

ROSE_INLINE void ui_draw_menu_back(ARegion *region, uiBlock *block, const rcti *rect) {
}

void UI_block_draw(const struct rContext *C, uiBlock *block) {
	ARegion *region = CTX_wm_region(C);

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
	if(lb) {
		if(region->runtime.block_name_map == NULL) {
			region->runtime.block_name_map = LIB_ghash_str_new(__func__);
		}
		oldblock = (uiBlock *)LIB_ghash_lookup(region->runtime.block_name_map, block->name);
		
		if(oldblock) {
			oldblock->active = false;
		}
		
		LIB_addhead(lb, block);
		LIB_ghash_reinsert(region->runtime.block_name_map, block->name, block, NULL, NULL);
	}
	block->oldblock = oldblock;
}

void UI_blocklist_free(const struct rContext *C, ARegion *region) {
	ListBase *lb = &region->uiblocks;
	for (uiBlock *block; block = (uiBlock *)LIB_pophead(lb);) {
		UI_block_free(C, block);
	}
	if(region->runtime.block_name_map != NULL) {
		LIB_ghash_free(region->runtime.block_name_map, NULL, NULL);
		region->runtime.block_name_map = NULL;
	}
}

void UI_blocklist_free_inactive(const struct rContext *C, ARegion *region) {
	ListBase *lb = &region->uiblocks;
	
	LISTBASE_FOREACH_MUTABLE(uiBlock *, block, lb) {
		if(block->active) {
			block->active = false;
		}
		else {
			if(region->runtime.block_name_map != NULL) {
				uiBlock *b = (uiBlock *)LIB_ghash_lookup(region->runtime.block_name_map, block->name);
				if(b == block) {
					LIB_ghash_remove(region->runtime.block_name_map, block->name, NULL, NULL);
				}
			}
			LIB_remlink(lb, block);
			UI_block_free(C, block);
		}
	}
}

void UI_block_free(const struct rContext *C, uiBlock *block) {
	for (uiBut *but; but = (uiBut *)LIB_pophead(&block->buttons);) {
		ui_but_free(C, but);
	}
	
	MEM_freeN(block->name);
	MEM_freeN(block);
}

/** \} */
