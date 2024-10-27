#ifndef KER_CONTEXT_H
#define KER_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

struct rContext;

/* -------------------------------------------------------------------- */
/** \name Create/Destroy Methods
 * \{ */

/** Create a new rose context for window manager and main database data-blocks. */
struct rContext *CTX_new(void);
/** Delete the context, this does not free any referenced data. */
void CTX_free(struct rContext *C);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Get/Set Methods
 * \{ */

struct WindowManager;
struct wmWindow;

struct Main;

struct WindowManager *CTX_wm_manager(const struct rContext *C);
struct wmWindow *CTX_wm_window(const struct rContext *C);
struct Main *CTX_data_main(const struct rContext *C);

void CTX_wm_manager_set(struct rContext *C, struct WindowManager *);
void CTX_wm_window_set(struct rContext *C, struct wmWindow *);
void CTX_data_main_set(struct rContext *C, struct Main *);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // KER_CONTEXT_H
