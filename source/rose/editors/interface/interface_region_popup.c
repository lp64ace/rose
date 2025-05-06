#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_resource.h"

#include "KER_context.h"
#include "KER_screen.h"

#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_math_vector.h"
#include "LIB_rect.h"
#include "LIB_string.h"
#include "LIB_string_utf.h"
#include "LIB_utildefines.h"

#include "GPU_framebuffer.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "interface_intern.h"

/* -------------------------------------------------------------------- */
/** \name Utility Functions
 * \{ */

ROSE_STATIC void ui_popup_block_position(wmWindow *window, ARegion *butregion, uiBut *but, uiBlock *block) {
	uiPopupBlockHandle *handle = block->handle;

	rctf butrct;
	ui_block_to_window_rctf(butregion, but->block, &butrct, &but->rect);

	if (block->rect.xmin == 0.0f && block->rect.xmax == 0.0f) {
		if (!LIB_listbase_is_empty(&block->buttons)) {
			LIB_rctf_init_minmax(&block->rect);

			LISTBASE_FOREACH(uiBut *, bt, &block->buttons) {
				LIB_rctf_union(&block->rect, &bt->rect);
			}
		}
		else {
			/* we're nice and allow empty blocks too */
			block->rect.xmin = block->rect.ymin = 0;
			block->rect.xmax = block->rect.ymax = 20;
		}
	}

	int delta = LIB_rctf_size_x(&block->rect) - LIB_rctf_size_x(&butrct);

	const float max_radius = (0.5f * WIDGET_UNIT);
	if (delta >= 0 && delta < max_radius) {
		LISTBASE_FOREACH(uiBut *, bt, &block->buttons) {
			/* Only trim the right most buttons in multi-column popovers. */
			if (bt->rect.xmax == block->rect.xmax) {
				bt->rect.xmax -= delta;
			}
		}
		block->rect.xmax -= delta;
	}

	ui_block_to_window_rctf(butregion, but->block, &block->rect, &block->rect);

	const int size_x = LIB_rctf_size_x(&block->rect);
	const int size_y = LIB_rctf_size_y(&block->rect);
	const int center_x = (block->direction & UI_DIR_CENTER_X) ? size_x / 2 : 0;
	const int center_y = (block->direction & UI_DIR_CENTER_Y) ? size_y / 2 : 0;

	const int win_x = WM_window_size_x(window);
	const int win_y = WM_window_size_y(window);

	const int max_size_x = ROSE_MAX(size_x, handle->max_size_x);
	const int max_size_y = ROSE_MAX(size_y, handle->max_size_y);

	int dir1 = 0, dir2 = 0;

	bool left = false, right = false, top = false, down = false;

	/* check if there's space at all */
	if (butrct.xmin - max_size_x + center_x > 0.0f) {
		left = true;
	}
	if (butrct.xmax + max_size_x - center_x < win_x) {
		right = true;
	}
	if (butrct.ymin - max_size_y + center_y > 0.0f) {
		down = true;
	}
	if (butrct.ymax + max_size_y - center_y < win_y) {
		top = true;
	}

	if (top == 0 && down == 0) {
		if (butrct.ymin - max_size_y < win_y - butrct.ymax - max_size_y) {
			top = true;
		}
		else {
			down = true;
		}
	}

	dir1 = (block->direction & UI_DIR_ALL);

	if (dir1 & (UI_DIR_UP | UI_DIR_DOWN)) {
		if (dir1 & UI_DIR_LEFT) {
			dir2 = UI_DIR_LEFT;
		}
		else if (dir1 & UI_DIR_RIGHT) {
			dir2 = UI_DIR_RIGHT;
		}
		dir1 &= (UI_DIR_UP | UI_DIR_DOWN);
	}

	if ((dir2 == 0) && ELEM(dir1, UI_DIR_LEFT, UI_DIR_RIGHT)) {
		dir2 = UI_DIR_DOWN;
	}
	if ((dir2 == 0) && ELEM(dir1, UI_DIR_UP, UI_DIR_DOWN)) {
		dir2 = UI_DIR_LEFT;
	}

	/* no space at all? don't change */
	if (left || right) {
		if (dir1 == UI_DIR_LEFT && left == 0) {
			dir1 = UI_DIR_RIGHT;
		}
		if (dir1 == UI_DIR_RIGHT && right == 0) {
			dir1 = UI_DIR_LEFT;
		}
		/* this is aligning, not append! */
		if (dir2 == UI_DIR_LEFT && right == 0) {
			dir2 = UI_DIR_RIGHT;
		}
		if (dir2 == UI_DIR_RIGHT && left == 0) {
			dir2 = UI_DIR_LEFT;
		}
	}

	if (down || top) {
		if (dir1 == UI_DIR_UP && top == 0) {
			dir1 = UI_DIR_DOWN;
		}
		if (dir1 == UI_DIR_DOWN && down == 0) {
			dir1 = UI_DIR_UP;
		}
		if (dir2 == UI_DIR_UP && top == 0) {
			dir2 = UI_DIR_DOWN;
		}
		if (dir2 == UI_DIR_DOWN && down == 0) {
			dir2 = UI_DIR_UP;
		}
	}

	/* Compute offset based on direction. */
	float offset_x = 0, offset_y = 0;

	if (dir1 == UI_DIR_LEFT) {
		offset_x = (butrct.xmin - block->rect.xmax) + PIXELSIZE;
		if (dir2 == UI_DIR_UP) {
			offset_y = butrct.ymin - block->rect.ymin - center_y - UI_MENU_PADDING;
		}
		else {
			offset_y = butrct.ymax - block->rect.ymax + center_y + UI_MENU_PADDING;
		}
	}
	else if (dir1 == UI_DIR_RIGHT) {
		offset_x = (butrct.xmax - block->rect.xmin) - PIXELSIZE;
		if (dir2 == UI_DIR_UP) {
			offset_y = butrct.ymin - block->rect.ymin - center_y - UI_MENU_PADDING;
		}
		else {
			offset_y = butrct.ymax - block->rect.ymax + center_y + UI_MENU_PADDING;
		}
	}
	else if (dir1 == UI_DIR_UP) {
		offset_y = (butrct.ymax - block->rect.ymin) - PIXELSIZE;
		if (dir2 == UI_DIR_RIGHT) {
			offset_x = butrct.xmax - block->rect.xmax + center_x;
		}
		else {
			offset_x = butrct.xmin - block->rect.xmin - center_x;
		}
	}
	else if (dir1 == UI_DIR_DOWN) {
		offset_y = (butrct.ymin - block->rect.ymax) + PIXELSIZE;
		if (dir2 == UI_DIR_RIGHT) {
			offset_x = butrct.xmax - block->rect.xmax + center_x;
		}
		else {
			offset_x = butrct.xmin - block->rect.xmin - center_x;
		}
	}

	/* Center over popovers for eg. */
	if (block->direction & UI_DIR_CENTER_X) {
		offset_x += LIB_rctf_size_x(&butrct) / ((dir2 == UI_DIR_LEFT) ? 2 : -2);
	}

	/* Apply offset, buttons in window coords. */
	LISTBASE_FOREACH(uiBut *, bt, &block->buttons) {
		ui_block_to_window_rctf(butregion, but->block, &bt->rect, &bt->rect);
		LIB_rctf_translate(&bt->rect, offset_x, offset_y);
		ui_but_update(bt);
	}

	LIB_rctf_translate(&block->rect, offset_x, offset_y);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Menu Block Creation
 * \{ */

ROSE_STATIC void ui_block_region_draw(struct rContext *C, ARegion *region) {
	float back[4];
	UI_GetThemeColor4fv(TH_BACK, back);
	GPU_clear_color(back[0], back[1], back[2], back[3]);
}

ROSE_STATIC void ui_popup_block_remove(struct rContext *C, uiPopupBlockHandle *handle) {
	wmWindow *ctx_win = CTX_wm_window(C);
	ScrArea *ctx_area = CTX_wm_area(C);
	ARegion *ctx_region = CTX_wm_region(C);

	Screen *screen = WM_window_screen_get(ctx_win);

	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *win = ctx_win;

	if (!LIB_haslink(&screen->regionbase, handle->region)) {
		LISTBASE_FOREACH(wmWindow *, win_iter, &wm->windows) {
			screen = WM_window_screen_get(win_iter);
			if (LIB_haslink(&screen->regionbase, handle->region)) {
				win = win_iter;
				break;
			}
		}
	}

	ROSE_assert(win && screen);

	CTX_wm_window_set(C, win);
	ui_region_temp_remove(C, screen, handle->region);

	CTX_wm_window_set(C, ctx_win);
	CTX_wm_area_set(C, ctx_area);
	CTX_wm_region_set(C, ctx_region);
}

ROSE_STATIC uiBlock *ui_popup_block_refresh(struct rContext *C, uiPopupBlockHandle *handle, ARegion *butregion, uiBut *but) {
	wmWindow *window = CTX_wm_window(C);
	ARegion *region = handle->region;

	uiBlock *block = NULL;
	if (!block) {
		if (handle->popup_create_vars.block_create_func) {
			block = handle->popup_create_vars.block_create_func(C, region, handle->popup_create_vars.arg);
		}
		else {
			block = handle->popup_create_vars.handle_create_func(C, handle, handle->popup_create_vars.arg);
		}
	}

	if (block->handle) {
		memcpy(block->handle, handle, sizeof(uiPopupBlockHandle));
		MEM_freeN(handle);
		handle = block->handle;
	}
	else {
		block->handle = handle;
	}

	if (!block->closed) {
		UI_block_end_ex(C, block, handle->popup_create_vars.event_xy, handle->popup_create_vars.event_xy);
	}

	if (but) {
		ui_popup_block_position(window, butregion, but, block);
	}

	region->winrct.xmin = block->rect.xmin;
	region->winrct.xmax = block->rect.xmax;
	region->winrct.ymin = block->rect.ymin;
	region->winrct.ymax = block->rect.ymax;

	UI_block_translate(block, -region->winrct.xmin, -region->winrct.ymin);

	ED_region_floating_init(region);
	ED_region_update_rect(region);

	WM_get_projection_matrix(block->winmat, &region->winrct);

	return block;
}

ROSE_STATIC void ui_block_region_refresh(struct rContext *C, ARegion *region) {
	ROSE_assert(region->regiontype == RGN_TYPE_TEMPORARY);

	ScrArea *prev_area = CTX_wm_area(C);
	ARegion *prev_region = CTX_wm_region(C);

	LISTBASE_FOREACH_MUTABLE(uiBlock *, block, &region->uiblocks) {
		uiPopupBlockHandle *handle = block->handle;

		if (handle->context.area) {
			CTX_wm_area_set(C, handle->context.area);
		}
		if (handle->context.region) {
			CTX_wm_region_set(C, handle->context.region);
		}

		ui_popup_block_refresh(C, handle, handle->popup_create_vars.butregion, handle->popup_create_vars.but);
	}

	CTX_wm_region_set(C, prev_region);
	CTX_wm_area_set(C, prev_area);
}

uiPopupBlockHandle *ui_popup_block_create(struct rContext *C, ARegion *butregion, uiBut *but, uiBlockCreateFunc block_create_fn, uiBlockHandleCreateFunc handle_create_fn, void *arg) {
	wmWindow *window = CTX_wm_window(C);

	uiPopupBlockHandle *handle = MEM_callocN(sizeof(uiPopupBlockHandle), "uiPopupBlockHandle");

	handle->context.area = CTX_wm_area(C);
	handle->context.region = CTX_wm_region(C);

	handle->popup_create_vars.block_create_func = block_create_fn;
	handle->popup_create_vars.handle_create_func = handle_create_fn;
	handle->popup_create_vars.arg = arg;
	handle->popup_create_vars.but = but;
	handle->popup_create_vars.butregion = but ? butregion : NULL;
	copy_v2_v2_int(handle->popup_create_vars.event_xy, window->event_state->mouse_xy);

	ARegion *region = ui_region_temp_add(CTX_wm_screen(C));
	handle->region = region;

	static ARegionType type;
	memset(&type, 0, sizeof(ARegionType));
	type.draw = ui_block_region_draw;
	type.layout = ui_block_region_refresh;
	type.regionid = RGN_TYPE_TEMPORARY;
	region->type = &type;

	UI_region_handlers_add(&region->handlers);

	uiBlock *block = ui_popup_block_refresh(C, handle, butregion, but);
	handle = block->handle;

	return handle;
}

void ui_popup_block_free(struct rContext *C, uiPopupBlockHandle *handle) {
	ui_popup_block_remove(C, handle);

	MEM_freeN(handle);
}

/** \} */
