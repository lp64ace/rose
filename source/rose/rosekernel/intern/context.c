#include "MEM_alloc.h"

#include "KER_context.h"

struct Context {
	struct {
		struct Main *main;
	} data;
	
	struct {
		struct wmWindowManager *manager;
		struct wmWindow *window;
	} wm;
};

struct Context *CTX_new() {
	Context *C = MEM_callocN(sizeof(Context), "kernel::Context");
	
	return C;
}

void CTX_free(struct Context *C) {
	MEM_freeN(C);
}

/* -------------------------------------------------------------------- */
/** \name Retrieve State Information
 * \{ */

struct Main *CTX_data_main(struct Context *C) {
	return C->data.main;
}

struct wmWindowManager *CTX_wm_manager(struct Context *C) {
	return C->wm.manager;
}

struct wmWindow *CTX_wm_window(struct Context *C) {
	return C->wm.window;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Set State Information
 * \{ */

void CTX_data_main_set(struct Context *C, struct Main *main) {
	C->data.main = main;
}

void CTX_wm_manager_set(struct Context *C, struct wmWindowManager *wm) {
	C->wm.manager = wm;
}

void CTX_wm_window_set(struct Context *C, struct wmWindow *win) {
	C->wm.window = win;
}

/* \} */
