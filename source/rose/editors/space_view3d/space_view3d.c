#include "ED_screen.h"
#include "ED_space_api.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "GPU_framebuffer.h"

#include "MEM_alloc.h"

#include "UI_interface.h"

#include "KER_screen.h"

#include <stdio.h>

static SpaceLink *view3d_create(const struct ScrArea *area) {
	ARegion *region;
	SpaceView3D *view3d;

	view3d = MEM_callocN(sizeof(SpaceView3D), "View3D::Link");
	view3d->spacetype = SPACE_VIEW3D;

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
}

static void view3d_main_region_draw(const struct Context *C, struct ARegion *region) {
	GPU_clear_color(0.35f, 0.35f, 0.35f, 1.0f);
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
