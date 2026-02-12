#include "MEM_guardedalloc.h"

#include "ED_screen.h"

#include "UI_interface.h"
#include "UI_resource.h"
#include "UI_view2d.h"

#include "KER_screen.h"

#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "LIB_linklist.h"
#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
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
	GPU_matrix_ortho_2d_set(0, region->sizex, 0, region->sizey);
	GPU_matrix_identity_set();
}

void ED_region_exit(rContext *C, ARegion *region) {
	WindowManager *wm = CTX_wm_manager(C);
	wmWindow *window = CTX_wm_window(C);
	ARegion *prevar = CTX_wm_region(C);

	if (region->type && region->type->exit) {
		region->type->exit(wm, region);
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

void ED_panel_do_draw(rContext *C, ARegion *region, ListBase *lb, PanelType *pt, Panel *panel, int w, int em) {
	uiBlock *block = UI_block_begin(C, region, pt->idname);

	int h = 0;

	panel = UI_panel_begin(region, lb, block, pt, panel);
	if (pt->draw_header && (!pt->flag & PANEL_TYPE_NO_HEADER)) {
		int labelx = 0, labely = 0;
		UI_panel_label_offset(block, &labelx, &labely);

		panel->layout = UI_block_layout(block, UI_LAYOUT_HORIZONTAL, ITEM_LAYOUT_ROW, labelx, labely, UI_UNIT_Y, BORDERPADDING);
		pt->draw_header(C, panel);
		panel->layout = NULL;
	}
	UI_panel_header_buttons_end(panel);

	panel->layout = UI_block_layout(block, UI_LAYOUT_VERTICAL, ITEM_LAYOUT_ROOT, 0, 0, 0, 0);
	pt->draw(C, panel);
	panel->layout = NULL;

	int cox, coy;
	UI_block_layout_resolve(block, &cox, &coy);
	UI_block_translate(block, 0, coy);

	if (coy) {
		h -= coy;
	}

	UI_block_end(C, block);

	LISTBASE_FOREACH(LinkData *, link, &pt->children) {
		PanelType *pt_child = (PanelType *)link->data;
		Panel *p_child = UI_panel_find_by_type(&panel->children, pt_child);

		if (pt_child->draw && (!pt_child->poll || pt_child->poll(C, pt_child))) {
			ED_panel_do_draw(C, region, &panel->children, pt_child, p_child, w, em);
		}
	}

	UI_panel_end(panel, w, h);
}

ROSE_INLINE bool panel_add_check(rContext *C, PanelType *pt) {
	/* Only add top level panels. */
	if (pt->parent) {
		return false;
	}

	if (pt->draw) {
		if (pt->poll && !pt->poll(C, pt)) {
			return false;
		}
	}

	return true;
}

ROSE_INLINE int panel_draw_width_from_max_width_get(const ARegion *region, const PanelType *panel_type, const int max_width) {
	/* With a background, we want some extra padding. */
	return UI_panel_should_show_background(region, panel_type) ? max_width - UI_UNIT_X * 2.0f : max_width;
}

void ED_region_panels_layout_ex(rContext *C, ARegion *region, ListBase *paneltypes) {
	LinkNode *panel_types_stack = NULL;
	LISTBASE_FOREACH_BACKWARD(PanelType *, pt, paneltypes) {
		if (panel_add_check(C, pt)) {
			LIB_linklist_prepend_alloca(&panel_types_stack, pt);
		}
	}

	if (!panel_types_stack) {
		return;
	}

	ScrArea *area = CTX_wm_area(C);
	View2D *v2d = &region->v2d;

	/* only allow scrolling in vertical direction */
	v2d->keepofs |= V2D_LOCKOFS_X | V2D_KEEPOFS_Y;
	v2d->keepofs &= ~(V2D_LOCKOFS_Y | V2D_KEEPOFS_X);
	v2d->scroll &= ~V2D_SCROLL_BOTTOM;

	if (region->alignment & RGN_ALIGN_LEFT) {
		region->v2d.scroll &= ~V2D_SCROLL_RIGHT;
		region->v2d.scroll |= V2D_SCROLL_LEFT;
	}
	else {
		region->v2d.scroll &= ~V2D_SCROLL_LEFT;
		region->v2d.scroll |= V2D_SCROLL_RIGHT;
	}

	const int max_panel_width = LIB_rctf_size_x(&v2d->cur);
	/* Works out to 10 * UI_UNIT_X or 20 * UI_UNIT_X. */
	const int em = 10;

	/* create panels */
	UI_panels_begin(C, region);
	UI_view2d_view_ortho(v2d);

	for (LinkNode *pt_link = panel_types_stack; pt_link; pt_link = pt_link->next) {
		PanelType *pt = (PanelType *)(pt_link->link);

		Panel *panel = UI_panel_find_by_type(&region->panels, pt);
		const int width = panel_draw_width_from_max_width_get(region, pt, max_panel_width);

		ED_panel_do_draw(C, region, &region->panels, pt, panel, width, em);
	}

	/* align panels and return size */
	int x, y;
	UI_panels_end(C, region, &x, &y);
}

void ED_region_panels_layout(rContext *C, ARegion *region) {
	ED_region_panels_layout_ex(C, region, &region->type->paneltypes);
}

void ED_region_do_layout(rContext *C, ARegion *region) {
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

	ED_region_panels_layout(C, region);

	UI_blocklist_free_inactive(C, region);

	GPU_matrix_pop_projection();
	GPU_matrix_pop();

	region->flag &= ~RGN_FLAG_LAYOUT;
}

ROSE_INLINE void region_clear(rContext *C, ARegion *region) {
	ScrArea *area = CTX_wm_area(C);
	
	float back[4];
	
	if (area && ED_area_is_global(area)) {
		UI_GetThemeColor4fv(TH_BACK_HI, back);
	}
	else {
		UI_GetThemeColor4fv(TH_BACK, back);
	}

	GPUTexture *texture = WM_draw_region_texture(region, 0);

	if (texture) {
		/**
		 * Using GPU_clear_color here will not work since Viewport does not bind a framebuffer!
		 */
		GPU_texture_clear(texture, GPU_DATA_FLOAT, back);
	}
}

void ED_region_panels_draw(rContext *C, ARegion *region) {
	View2D *v2d = &region->v2d;
	const float aspect = LIB_rctf_size_y(&region->v2d.cur) / (LIB_rcti_size_y(&region->v2d.mask) + 1);

	/* reset line width for drawing tabs */
	GPU_line_width(1.0f);

	/* set the view */
	UI_view2d_view_ortho(v2d);

	/* View2D matrix might have changed due to dynamic sized regions. */
	UI_panels_draw(C, region);
}

void ED_region_do_draw(rContext *C, ARegion *region) {
	if ((region->flag & (RGN_FLAG_REDRAW | RGN_FLAG_ALWAYS_REDRAW)) == 0) {
		return;
	}

	ScrArea *area = CTX_wm_area(C);

	UI_SetTheme((area) ? area->spacetype : SPACE_EMPTY, region->regiontype);

	GPU_matrix_push();
	GPU_matrix_push_projection();

	region_clear(C, region);
	
	if (!LIB_listbase_is_empty(&region->panels)) {
		ED_region_panels_draw(C, region);
	}

	ED_region_pixelspace(region);

	if (region->type->draw) {
		region->type->draw(C, region);
	}

	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		if (block->panel == NULL) {
			UI_block_draw(C, block);
		}
	}

	GPU_matrix_pop_projection();
	GPU_matrix_pop();

	region->flag &= ~RGN_FLAG_REDRAW;
}

void ED_region_header_init(WindowManager *wm, ARegion *region) {
	UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_HEADER, region->sizex, region->sizey);
}

void ED_region_header_exit(WindowManager *wm, ARegion *region) {
}

void ED_region_header_draw(rContext *C, ARegion *region) {
}

void ED_region_default_init(WindowManager *wm, ARegion *region) {
	UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_STANDARD, region->sizex, region->sizey);
}

void ED_region_default_exit(WindowManager *wm, ARegion *region) {
}

void ED_region_default_draw(rContext *C, ARegion *region) {
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

/* -------------------------------------------------------------------- */
/** \name Panel
 * \{ */

void ED_region_panels_init(struct WindowManager *wm, struct ARegion *region) {
	UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_PANELS_UI, region->sizex, region->sizey);

	/* Place scroll bars to the left if left-aligned, right if right-aligned. */
	if (region->alignment & RGN_ALIGN_LEFT) {
		region->v2d.scroll &= ~V2D_SCROLL_RIGHT;
		region->v2d.scroll |= V2D_SCROLL_LEFT;
	}
	else if (region->alignment & RGN_ALIGN_RIGHT) {
		region->v2d.scroll &= ~V2D_SCROLL_LEFT;
		region->v2d.scroll |= V2D_SCROLL_RIGHT;
	}

	wmKeyMap *keymap = WM_keymap_ensure(wm->runtime.defaultconf, "View2D Buttons List", SPACE_EMPTY, RGN_TYPE_WINDOW);
	WM_event_add_keymap_handler(&region->handlers, keymap);
}

/** \} */
