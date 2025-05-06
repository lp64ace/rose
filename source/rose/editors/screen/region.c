#include "MEM_guardedalloc.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_resource.h"
#include "UI_view2d.h"

#include "KER_screen.h"

#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "WM_draw.h"
#include "WM_handler.h"

#include "screen_intern.h"

/* -------------------------------------------------------------------- */
/** \name Region
 * \{ */

void ED_region_floating_init(ARegion *region) {
	region->visible = true;
}

void ED_region_pixelspace(ARegion *region) {
	GPU_matrix_ortho_2d_set(0.0f, region->sizex, 0.0f, region->sizey);
}

void ED_region_exit(struct rContext *C, ARegion *region) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = CTX_wm_window(C);
	ARegion *prevar = CTX_wm_region(C);

	if (region->type && region->type->exit) {
		region->type->exit(region);
	}

	CTX_wm_region_set(C, region);

	WM_event_remove_handlers(C, &region->handlers);
	WM_event_modal_handler_region_replace(window, region, NULL);

	UI_region_free_active_but_all(C, region);

	WM_draw_region_free(region);
	region->visible = false;

	UI_blocklist_free(C, region);

	CTX_wm_region_set(C, prevar);
}

void ED_region_update_rect(ARegion *region) {
	region->sizex = LIB_rcti_size_x(&region->winrct);
	region->sizey = LIB_rcti_size_y(&region->winrct);
}

void ED_region_do_layout(struct rContext *C, ARegion *region) {
	if ((region->flag & (RGN_FLAG_LAYOUT | RGN_FLAG_ALWAYS_REBUILD)) == 0) {
		return;
	}

	GPU_matrix_push();
	GPU_matrix_push_projection();
	GPU_matrix_identity_set();

	ED_region_pixelspace(region);

	if (region->type && region->type->layout) {
		region->type->layout(C, region);
	}

	UI_blocklist_free_inactive(C, region);

	GPU_matrix_pop_projection();
	GPU_matrix_pop();

	region->flag &= ~RGN_FLAG_LAYOUT;
}

ROSE_STATIC void region_clear(struct rContext *C, ARegion *region) {
	ScrArea *area = CTX_wm_area(C);
	
	float back[4];
	
	if (area && ED_area_is_global(area)) {
		UI_GetThemeColor4fv(TH_BACK_HI, back);
	}
	else {
		UI_GetThemeColor4fv(TH_BACK, back);
	}
	
	GPU_clear_color(back[0], back[1], back[2], back[3]);
}

void ED_region_do_draw(struct rContext *C, ARegion *region) {
	if ((region->flag & (RGN_FLAG_REDRAW | RGN_FLAG_ALWAYS_REDRAW)) == 0) {
		return;
	}

	ScrArea *area = CTX_wm_area(C);

	UI_SetTheme((area) ? area->spacetype : SPACE_EMPTY, region->regiontype);

	GPU_matrix_push();
	GPU_matrix_push_projection();
	GPU_matrix_identity_set();

	region_clear(C, region);
	
	ED_region_pixelspace(region);

	if (region->type && region->type->draw) {
		region->type->draw(C, region);
	}

	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		UI_block_draw(C, block);
	}

	GPU_matrix_pop_projection();
	GPU_matrix_pop();

	region->flag &= ~RGN_FLAG_REDRAW;
}

void ED_region_header_init(ARegion *region) {
	UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_HEADER, region->sizex, region->sizey);
}

void ED_region_header_exit(ARegion *region) {
}

void ED_region_header_draw(struct rContext *C, ARegion *region) {
}

void ED_region_default_init(ARegion *region) {
	UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_STANDARD, region->sizex, region->sizey);
}

void ED_region_default_exit(ARegion *region) {
}

void ED_region_default_draw(struct rContext *C, ARegion *region) {
}

bool ED_region_contains_xy(const ARegion *region, const int event_xy[2]) {
	if (LIB_rcti_isect_pt_v(&region->winrct, event_xy)) {
		ROSE_assert(!region->overlap);
		return true;
	}
	return false;
}

struct ARegion *ED_area_find_region_xy_visual(const struct ScrArea *area, int regiontype, const int event_xy[2]) {
	if (!area) {
		return NULL;
	}

	/* Check overlapped regions first. */
	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		if (!region->overlap) {
			continue;
		}
		if (ELEM(regiontype, RGN_TYPE_ANY, region->regiontype)) {
			if (ED_region_contains_xy(region, event_xy)) {
				return region;
			}
		}
	}
	/* Now non-overlapping ones. */
	LISTBASE_FOREACH(ARegion *, region, &area->regionbase) {
		if (region->overlap) {
			continue;
		}
		if (ELEM(regiontype, RGN_TYPE_ANY, region->regiontype)) {
			if (ED_region_contains_xy(region, event_xy)) {
				return region;
			}
		}
	}
	return NULL;
}

void ED_region_tag_redraw(ARegion *region) {
	region->flag |= RGN_FLAG_LAYOUT;
	region->flag |= RGN_FLAG_REDRAW;
}

void ED_region_tag_redraw_no_rebuild(ARegion *region) {
	region->flag |= RGN_FLAG_REDRAW;
}

/** \} */
