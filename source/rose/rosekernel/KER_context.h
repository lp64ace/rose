#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Context Context;

/**
 * Create a new context for this application.
 * The context for this application, this stores runtime information about the state of the application.
 */
struct Context *CTX_new();

/**
 * Free the applcation's context, this does not free the elements inside this context.
 */
void CTX_free(struct Context *C);

struct Main;

/* -------------------------------------------------------------------- */
/** \name Retrieve State Information
 * \{ */

struct Main *CTX_data_main(struct Context *C);

struct wmWindowManager *CTX_wm_manager(struct Context *C);
struct wmWindow *CTX_wm_window(struct Context *C);
struct WorkSpace *CTX_wm_workspace(const Context *C);
struct Screen *CTX_wm_screen(const Context *C);
struct ScrArea *CTX_wm_area(const Context *C);
struct ARegion *CTX_wm_region(const Context *C);
struct ARegion *CTX_wm_menu(const Context *C);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Set State Information
 * \{ */

void CTX_data_main_set(struct Context *C, struct Main *main);

void CTX_wm_manager_set(struct Context *C, struct wmWindowManager *wm);
void CTX_wm_window_set(struct Context *C, struct wmWindow *win);
void CTX_wm_workspace_set(struct Context *C, struct WorkSpace *workspace);
void CTX_wm_screen_set(struct Context *C, struct Screen *screen);
void CTX_wm_area_set(struct Context *C, struct ScrArea *area);
void CTX_wm_region_set(struct Context *C, struct ARegion *region);
void CTX_wm_menu_set(struct Context *C, struct ARegion *menu);

/* \} */

#ifdef __cplusplus
}
#endif
