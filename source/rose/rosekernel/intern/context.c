#include "MEM_guardedalloc.h"

#include "KER_context.h"

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct rContext {
	struct {
		struct WindowManager *manager;
		struct wmWindow *window;
	} wm;
	
	struct {
		struct Main *main;
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
struct Main *CTX_data_main(const rContext *ctx) {
	return ctx->data.main;
}

void CTX_wm_manager_set(rContext *ctx, struct WindowManager *manager) {
	ctx->wm.manager = manager;
}
void CTX_wm_window_set(rContext *ctx, struct wmWindow *window) {
	ctx->wm.window = window;
}
void CTX_data_main_set(rContext *ctx, struct Main *main) {
	ctx->data.main = main;
}

/** \} */
