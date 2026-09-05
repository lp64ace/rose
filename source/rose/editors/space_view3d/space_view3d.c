#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_view3d_types.h"
#include "DNA_windowmanager_types.h"

#include "GPU_framebuffer.h"
#include "GPU_texture.h"
#include "GPU_matrix.h"
#include "GPU_viewport.h"

#include "DRW_engine.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "UI_interface.h"

#include "LIB_listbase.h"
#include "LIB_math_geom.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "WM_api.h"
#include "WM_draw.h"

#include "KER_screen.h"

#include "view3d_intern.h"

/* -------------------------------------------------------------------- */
/** \name View3D SpaceType Methods
 * \{ */

ROSE_INLINE void view3d_viewplane_get(ARegion *region, rctf *r_viewplane, const float near, const float far) {
	/* default, human vertical fov is 120 degrees. */
	const float fov = M_PI_2 * 2.0f / 3.0f;

	float tangent = tan(fov * 0.5f);
	float aspect = (float)region->sizex / (float)region->sizey;

	r_viewplane->xmin = -tangent * aspect * near;
	r_viewplane->xmax = +tangent * aspect * near;
	r_viewplane->ymin = -tangent * near;
	r_viewplane->ymax = +tangent * near;
}

ROSE_INLINE void view3d_window_matrix(ARegion *region, float r_winmat[4][4], const rcti *rect) {
	const float near = 1e-1f;
	const float far = 1e+3f;

	rctf fullplane;
	view3d_viewplane_get(region, &fullplane, near, far);

	rctf viewplane;
	if (rect) {
		/* Smaller viewplane subset for selection picking. */
		viewplane.xmin = fullplane.xmin + (LIB_rctf_size_x(&fullplane) * ((float)rect->xmin / (float)region->sizex));
		viewplane.ymin = fullplane.ymin + (LIB_rctf_size_y(&fullplane) * ((float)rect->ymin / (float)region->sizey));
		viewplane.xmax = fullplane.xmin + (LIB_rctf_size_x(&fullplane) * ((float)rect->xmax / (float)region->sizex));
		viewplane.ymax = fullplane.ymin + (LIB_rctf_size_y(&fullplane) * ((float)rect->ymax / (float)region->sizey));
	}
	else {
		memcpy(&viewplane, &fullplane, sizeof(rctf));
	}

	perspective_m4(r_winmat, viewplane.xmin, viewplane.xmax, viewplane.ymin, viewplane.ymax, near, far);
}

ROSE_INLINE RegionView3D *region_view3d_init(RegionView3D *rv3d) {
	unit_m4(rv3d->winmat);
	unit_m4(rv3d->viewmat);
	unit_qt(rv3d->viewquat);
	copy_v3_fl3(rv3d->viewloc, 0.0f, 1.80f, 4.0f);
	return rv3d;
}

ROSE_INLINE SpaceLink *view3d_create(const ScrArea *area) {
	View3D *view3d = MEM_callocN(sizeof(View3D), "SpaceLink::View3D");

	// Main Region
	{
		ARegion *region = MEM_callocN(sizeof(ARegion), "View3D::Main");
		LIB_addtail(&view3d->regionbase, region);
		region->regiontype = RGN_TYPE_WINDOW;

		RegionView3D *rv3d = MEM_callocN(sizeof(RegionView3D), "RegionView3D");
		region->regiondata = region_view3d_init(rv3d);
		region->flag |= RGN_FLAG_ALWAYS_REDRAW;
	}
	view3d->spacetype = SPACE_VIEW3D;

	return (SpaceLink *)view3d;
}

ROSE_INLINE void view3d_free(SpaceLink *link) {
}

ROSE_INLINE void view3d_init(WindowManager *wm, ScrArea *area) {
}

ROSE_INLINE void view3d_exit(WindowManager *wm, ScrArea *area) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name View3D Main Region Methods
 * \{ */

ROSE_INLINE void view3d_main_region_init(WindowManager *wm, ARegion *region) {
	RegionView3D *rv3d = (RegionView3D *)region->regiondata;
	wmKeyMap *keymap;
	
	if ((keymap = WM_keymap_ensure(wm->runtime->defaultconf, "3D View", SPACE_VIEW3D, RGN_TYPE_WINDOW)) != NULL) {
		WM_event_add_keymap_handler(&region->handlers, keymap);
	}

	ED_region_default_init(wm, region);

	view3d_window_matrix(region, rv3d->winmat, NULL);
}

ROSE_INLINE void view3d_main_region_layout(rContext *C, ARegion *region) {
	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "VIEW3D_block"))) {
		UI_block_end(C, block);
	}
}

ROSE_INLINE void view3d_main_region_setup_view(ARegion *region, RegionView3D *rv3d, const float viewmat[4][4], const float winmat[4][4], rcti *rect) {
	if (winmat) {
		copy_m4_m4(rv3d->winmat, winmat);
	}
	else {
		view3d_window_matrix(region, rv3d->winmat, rect);
	}

	if (viewmat) {
		copy_m4_m4(rv3d->viewmat, viewmat);
	}
	else {
		quat_to_mat4(rv3d->viewmat, rv3d->viewquat);
		add_v3_v3(rv3d->viewmat[3], rv3d->viewloc);
		invert_m4(rv3d->viewmat);
	}
}

void ED_view3d_draw_setup_view(ARegion *region, const float viewmat[4][4], const float winmat[4][4], rcti *rect) {
	RegionView3D *rv3d = (RegionView3D *)region->regiondata;

	view3d_main_region_setup_view(region, rv3d, viewmat, winmat, rect);
}

ROSE_INLINE void view3d_main_region_draw(rContext *C, ARegion *region) {
	GPU_matrix_push();
	GPU_matrix_identity_set();
	GPU_matrix_push_projection();
	GPU_matrix_identity_projection_set();

	ED_view3d_draw_setup_view(region, NULL, NULL, NULL);

	DRW_draw_view(C);

	GPU_matrix_pop_projection();
	GPU_matrix_pop();
}

ROSE_INLINE void view3d_main_region_free(struct ARegion *region) {
	RegionView3D *rv3d = region->regiondata;

	region->regiondata = NULL;
	MEM_SAFE_FREE(rv3d);
}

/** \} */

void ED_spacetype_view3d() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "SpaceType::View3D");

	st->spaceid = SPACE_VIEW3D;
	LIB_strcpy(st->name, ARRAY_SIZE(st->name), "View3D");

	st->create = view3d_create;
	st->free = view3d_free;
	st->init = view3d_init;
	st->exit = view3d_exit;
	st->operatortypes = view3d_operatortypes;
	st->keymap = view3d_keymap;
	st->keymapflag = ED_KEYMAP_UI;

	// Header Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "View3D::ARegionType::Header");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_HEADER;
		art->draw = NULL;
		art->init = ED_region_header_init;
		art->exit = ED_region_header_exit;
		art->keymapflag = ED_KEYMAP_UI;
	}
	// Main Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "View3D::ARegionType::Main");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_WINDOW;
		art->layout = NULL;
		art->draw = view3d_main_region_draw;
		art->init = view3d_main_region_init;
		art->exit = ED_region_default_exit;
		art->free = view3d_main_region_free;
		art->keymapflag = ED_KEYMAP_UI;
	}

	KER_spacetype_register(st);
}
