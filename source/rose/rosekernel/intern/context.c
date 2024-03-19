#include "MEM_alloc.h"

#include "KER_context.h"

struct Context {
	struct {
		struct Main *main;
	} data;

	struct {
		struct wmWindowManager *manager;
		struct wmWindow *window;
		struct WorkSpace *workspace;
		struct Screen *screen;
		struct ScrArea *area;
		struct ARegion *region;
		struct ARegion *menu;
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

struct WorkSpace *CTX_wm_workspace(const Context *C) {
	return C->wm.workspace;
}

struct Screen *CTX_wm_screen(const Context *C) {
	return C->wm.screen;
}

struct ScrArea *CTX_wm_area(const Context *C) {
	return C->wm.area;
}

struct ARegion *CTX_wm_region(const Context *C) {
	return C->wm.region;
}

struct ARegion *CTX_wm_menu(const Context *C) {
	return C->wm.menu;
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

void CTX_wm_screen_set(struct Context *C, struct Screen *screen) {
	C->wm.screen = screen;
}

void CTX_wm_area_set(struct Context *C, struct ScrArea *area) {
	C->wm.area = area;
}

void CTX_wm_region_set(struct Context *C, struct ARegion *region) {
	C->wm.region = region;
}

void CTX_wm_menu_set(struct Context *C, struct ARegion *menu) {
	C->wm.menu = menu;
}

/* \} */
