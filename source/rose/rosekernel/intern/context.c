#include "MEM_guardedalloc.h"

#include "KER_context.h"
#include "KER_layer.h"
#include "KER_scene.h"

#include "DNA_windowmanager_types.h"

#include "DEG_depsgraph.h"

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
struct ViewLayer *CTX_data_view_layer(const rContext *C) {
	ViewLayer *view_layer = NULL;

	wmWindow *window = CTX_wm_window(C);
	Scene *scene = CTX_data_scene(C);
	if (window) {
		if ((view_layer = KER_view_layer_find(scene, window->view_layer_name)) != NULL) {
			return view_layer;
		}
	}

	return KER_view_layer_default_view(scene);
}

Depsgraph *CTX_data_depsgraph_pointer(const rContext *C) {
	Main *main = CTX_data_main(C);
	Scene *scene = CTX_data_scene(C);
	ViewLayer *view_layer = CTX_data_view_layer(C);
	Depsgraph *depsgraph = KER_scene_ensure_depsgraph(main, scene, view_layer);
	/**
	 * Dependency graph might have been just allocated, and hence it will not be marked.
	 * This confuses redo system due to the lack of flushing changes back to the original data.
	 * In the future we would need to check whether the CTX_wm_window(C)  is in editing mode (as an
	 * opposite of playback-preview-only) and set active flag based on that.
	 */
	DEG_make_active(depsgraph);
	return depsgraph;
}

Depsgraph *CTX_data_expect_evaluated_depsgraph(const rContext *C) {
	Depsgraph *depsgraph = CTX_data_depsgraph_pointer(C);
	/* TODO(lp64ace): Assert that the dependency graph is fully evaluated.
	 * Note that first the depsgraph and scene post-eval hooks needs to run extra round of updates
	 * first to make check here really reliable. */
	return depsgraph;
}

Depsgraph *CTX_data_ensure_evaluated_depsgraph(const rContext *C) {
	Depsgraph *depsgraph = CTX_data_depsgraph_pointer(C);
	Main *main = CTX_data_main(C);

	KER_scene_graph_evaluated_ensure(depsgraph, main);

	return depsgraph;
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
