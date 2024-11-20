#include "MEM_guardedalloc.h"

#include "ED_screen.h"
#include "UI_interface.h"

#include "KER_screen.h"

#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_state.h"

#include "LIB_listbase.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "WM_draw.h"

#include "screen_intern.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Region
 * \{ */

void ED_region_pixelspace(ARegion *region) {
	GPU_matrix_ortho_2d_set(0.0f, region->sizex, 0.0f, region->sizey);
}

void ED_region_exit(struct rContext *C, ARegion *region) {
	ARegion *prevar = CTX_wm_region(C);

	if (region->type && region->type->exit) {
		region->type->exit(region);
	}

	CTX_wm_region_set(C, region);

	WM_draw_region_free(region);
	region->visible = false;

	UI_blocklist_free(C, region);
	
	CTX_wm_region_set(C, prevar);
}
void ED_region_do_draw(struct rContext *C, struct ARegion *region) {
	GPU_matrix_push();
	GPU_matrix_push_projection();
	GPU_matrix_identity_set();
	
	ED_region_pixelspace(region);

	if (region->type && region->type->draw) {
		region->type->draw(C, region);
	}

	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		UI_block_draw(C, block);
	}
	UI_blocklist_free_inactive(C, region);

	GPU_matrix_pop_projection();
	GPU_matrix_pop();
}

void ED_region_header_init(ARegion *region) {
}
void ED_region_header_exit(ARegion *region) {
}
void ED_region_header_draw(struct rContext *C, ARegion *region) {
	GPU_clear_color(0.65f, 0.65f, 0.65f, 1.0f);
}

/** \} */
