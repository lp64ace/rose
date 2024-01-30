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

/* \} */

/* -------------------------------------------------------------------- */
/** \name Set State Information
 * \{ */

void CTX_data_main_set(struct Context *C, struct Main *main);

void CTX_wm_manager_set(struct Context *C, struct wmWindowManager *wm);
void CTX_wm_window_set(struct Context *C, struct wmWindow *win);

/* \} */

#ifdef __cplusplus
}
#endif
