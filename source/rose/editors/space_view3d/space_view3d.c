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

ROSE_INLINE void view3d_window_matrix(ARegion *region, float r_winmat[4][4]) {
	/* default, human vertical fov is 120 degrees. */
	const float fov = M_PI_2 * 2.0f / 3.0f;
	const float clip_start = 1e-1f;
	const float clip_end = 1e+3f;

	float tangent = tanf(fov * 0.5f);
	float aspect = (float)region->sizex / (float)region->sizey;

	rctf viewplane;
	viewplane.xmin = -tangent * aspect * clip_start;
	viewplane.xmax = +tangent * aspect * clip_start;
	viewplane.ymin = -tangent * clip_start;
	viewplane.ymax = +tangent * clip_start;

	perspective_m4(r_winmat, viewplane.xmin, viewplane.xmax, viewplane.ymin, viewplane.ymax, clip_start, clip_end);
}

ROSE_INLINE RegionView3D *region_view3d_init(RegionView3D *rv3d) {
	unit_m4(rv3d->winmat);
	unit_m4(rv3d->viewmat);
	unit_qt(rv3d->viewquat);
	copy_v3_fl3(rv3d->viewloc, 0.0f, 1.0f, 1.0f);
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
	
	if ((keymap = WM_keymap_ensure(wm->runtime.defaultconf, "3D View", SPACE_VIEW3D, RGN_TYPE_WINDOW)) != NULL) {
		WM_event_add_keymap_handler(&region->handlers, keymap);
	}

	ED_region_default_init(wm, region);

	view3d_window_matrix(region, rv3d->winmat);
}

ROSE_INLINE void view3d_main_region_layout(rContext *C, ARegion *region) {
	uiBlock *block;
	uiBut *but;
	if ((block = UI_block_begin(C, region, "VIEW3D_block"))) {
		UI_block_end(C, block);
	}
}

ROSE_INLINE void view3d_viewmatrix_set(RegionView3D *rv3d) {
	quat_to_mat4(rv3d->viewmat, rv3d->viewquat);
	add_v3_v3(rv3d->viewmat[3], rv3d->viewloc);
	invert_m4(rv3d->viewmat);
}

ROSE_INLINE void view3d_main_region_draw(rContext *C, ARegion *region) {
	RegionView3D *rv3d = (RegionView3D *)region->regiondata;

	GPU_matrix_push();
	GPU_matrix_identity_set();
	GPU_matrix_push_projection();
	GPU_matrix_identity_projection_set();

	view3d_viewmatrix_set(rv3d);

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

	// Header Region
	{
		ARegionType *art = MEM_callocN(sizeof(ARegionType), "View3D::ARegionType::Header");
		LIB_addtail(&st->regiontypes, art);
		art->regionid = RGN_TYPE_HEADER;
		art->draw = NULL;
		art->init = ED_region_header_init;
		art->exit = ED_region_header_exit;
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
	}

	KER_spacetype_register(st);
}
