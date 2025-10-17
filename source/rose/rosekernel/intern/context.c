#include "MEM_guardedalloc.h"

#include "KER_context.h"

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct rContext {
	struct {
		struct WindowManager *manager;
		struct wmWindow *window;
		struct Screen *screen;
		struct ScrArea *area;
		struct ARegion *region;
	} wm;

	struct {
		struct Main *main;
		struct Scene *scene;
	} data;
} rContext;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create/Destroy Methods
 * \{ */

rContext *CTX_new(void) {
	rContext *ctx = MEM_callocN(sizeof(rContext), "rContext");

	return ctx;
}
void CTX_free(rContext *ctx) {
	MEM_freeN(ctx);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Get/Set Methods
 * \{ */

struct WindowManager *CTX_wm_manager(const rContext *ctx) {
	return ctx->wm.manager;
}
struct wmWindow *CTX_wm_window(const rContext *ctx) {
	return ctx->wm.window;
}
struct Screen *CTX_wm_screen(const rContext *ctx) {
	return ctx->wm.screen;
}
struct ScrArea *CTX_wm_area(const rContext *ctx) {
	return ctx->wm.area;
}
struct ARegion *CTX_wm_region(const rContext *ctx) {
	return ctx->wm.region;
}
struct Main *CTX_data_main(const rContext *ctx) {
	return ctx->data.main;
}
struct Scene *CTX_data_scene(const rContext *ctx) {
	return ctx->data.scene;
}

void CTX_wm_manager_set(rContext *ctx, struct WindowManager *manager) {
	ctx->wm.manager = manager;
}
void CTX_wm_window_set(rContext *ctx, struct wmWindow *window) {
	ctx->wm.window = window;
}
void CTX_wm_screen_set(rContext *ctx, struct Screen *screen) {
	ctx->wm.screen = screen;
}
void CTX_wm_area_set(rContext *ctx, struct ScrArea *area) {
	ctx->wm.area = area;
}
void CTX_wm_region_set(rContext *ctx, struct ARegion *region) {
	ctx->wm.region = region;
}
void CTX_data_main_set(rContext *ctx, struct Main *main) {
	ctx->data.main = main;
}
void CTX_data_scene_set(rContext *ctx, struct Scene *scene) {
	ctx->data.scene = scene;
}

/** \} */
