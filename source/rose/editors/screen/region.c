#include "MEM_guardedalloc.h"

#include "ED_screen.h"

#include "KER_screen.h"

#include "GPU_framebuffer.h"

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "WM_draw.h"

#include "screen_intern.h"

/* -------------------------------------------------------------------- */
/** \name Region
 * \{ */

void ED_region_exit(struct rContext *C, ARegion *region) {
	ARegion *prevar = CTX_wm_region(C);
	
	if (region->type && region->type->exit) {
		region->type->exit(region);
	}
	
	CTX_wm_region_set(C, region);
	
	WM_draw_region_free(region);
	region->visible = false;
	
	CTX_wm_region_set(C, prevar);
}
void ED_region_do_draw(struct rContext *C, struct ARegion *region) {
	if (region->type && region->type->draw) {
		region->type->draw(C, region);
	}
}

void ED_region_header_init(ARegion *region) {
}
void ED_region_header_exit(ARegion *region) {
}
void ED_region_header_draw(struct rContext *C, ARegion *region) {
	GPU_clear_color(0.65f, 0.65f, 0.65f, 1.0f);
}

/** \} */
