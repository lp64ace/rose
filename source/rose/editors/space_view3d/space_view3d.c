#include "ED_screen.h"
#include "ED_space_api.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_math.h"
#include "LIB_string.h"
#include "LIB_time.h"
#include "LIB_utildefines.h"

#include "GPU_framebuffer.h"

#include "MEM_alloc.h"

#include "UI_interface.h"
#include "UI_view2d.h"

#include "RFT_api.h"

#include "KER_context.h"
#include "KER_screen.h"

#include <stdio.h>

static SpaceLink *view3d_create(const struct ScrArea *area) {
	ARegion *region;
	SpaceView3D *view3d;

	view3d = MEM_callocN(sizeof(SpaceView3D), "View3D::Link");
	view3d->spacetype = SPACE_VIEW3D;
	view3d->last_drw_time = _check_seconds_timer_float();

	region = MEM_callocN(sizeof(ARegion), "View3D::main");
	LIB_addtail(&view3d->regionbase, region);
	region->regiontype = RGN_TYPE_WINDOW;

	return (SpaceLink *)view3d;
}

static void view3d_free(const struct SpaceLink *link) {
}

static void view3d_init(const struct wmWindowManager *wm, struct ScrArea *area) {
}

static SpaceLink *view3d_duplicate(const struct SpaceLink *sl) {
	SpaceTopBar *view3d = MEM_dupallocN(sl);

	return (SpaceLink *)view3d;
}

static void view3d_main_region_init(struct wmWindowManager *wm, struct ARegion *region) {
	UI_view2d_region_reinit(&region->v2d, V2D_COMMONVIEW_STANDARD, region->winx, region->winy);
}

ROSE_INLINE int view3d_main_region_overlay_font_init(struct ARegion *region) {
	int fontid = RFT_default();

	RFT_color3f(fontid, 1.75f, 0.75f, 0.5f);
	RFT_size(fontid, 11);
	
	return fontid;
}

/** Draw overlay text to the top right corner of the main region */
ROSE_INLINE void view3d_main_region_draw_overlay_text(struct ARegion *region, const char *text) {
	const int fontid = view3d_main_region_overlay_font_init(region), pad = 4;
	rcti boundbox;

	RFT_boundbox(fontid, text, -1, &boundbox);
	RFT_position(fontid, region->winx - (LIB_rcti_size_x(&boundbox) + pad), region->winy - (LIB_rcti_size_y(&boundbox) + pad), 0);
	RFT_draw(fontid, text, -1);
}

ROSE_INLINE void view3d_main_region_draw_overlay(struct ARegion *region, struct SpaceView3D *view3d) {
	const float now = _check_seconds_timer_float();
	const float dt = now - view3d->last_drw_time;

	char *text = LIB_sprintf_allocN("%.1f", (1.0f / dt));
	if (text) {
		view3d_main_region_draw_overlay_text(region, text);
		MEM_freeN(text);
	}
	view3d->last_drw_time = now;
}

static void view3d_main_region_draw(const struct Context *C, struct ARegion *region) {
	GPU_clear_color(0.25f, 0.25f, 0.25f, 1.0f);

	struct ScrArea *area = CTX_wm_area(C);
	struct SpaceView3D *view3d = (struct SpaceView3D *)area->spacedata.first;

	view3d_main_region_draw_overlay(region, view3d);
}

void ED_spacetype_view3d() {
	SpaceType *st = MEM_callocN(sizeof(SpaceType), "rose::spacetype::View3D");

	ARegionType *art;

	st->spaceid = SPACE_VIEW3D;
	LIB_strncpy(st->name, "View3D", ARRAY_SIZE(st->name));

	st->create = view3d_create;
	st->free = view3d_free;
	st->init = view3d_init;
	st->duplicate = view3d_duplicate;

	art = MEM_callocN(sizeof(ARegionType), "rose::regiontype::View3D::MainRegion");
	art->regionid = RGN_TYPE_WINDOW;
	art->init = view3d_main_region_init;
	art->draw = view3d_main_region_draw;

	LIB_addhead(&st->regiontypes, art);

	KER_spacetype_register(st);
}
