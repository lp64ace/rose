#ifndef KER_CONTEXT_H
#define KER_CONTEXT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rContext rContext;

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

struct ARegion;
struct ScrArea;
struct Screen;
struct WindowManager;
struct wmWindow;

struct Depsgraph;
struct Main;
struct Scene;
struct ViewLayer;

struct WindowManager *CTX_wm_manager(const struct rContext *C);
struct wmWindow *CTX_wm_window(const struct rContext *C);
struct Screen *CTX_wm_screen(const struct rContext *C);
struct ScrArea *CTX_wm_area(const struct rContext *C);
struct ARegion *CTX_wm_region(const struct rContext *C);
struct Main *CTX_data_main(const struct rContext *C);
struct Scene *CTX_data_scene(const struct rContext *C);
struct ViewLayer *CTX_data_view_layer(const struct rContext *C);

void CTX_wm_manager_set(struct rContext *C, struct WindowManager *);
void CTX_wm_window_set(struct rContext *C, struct wmWindow *);
void CTX_wm_screen_set(struct rContext *C, struct Screen *);
void CTX_wm_area_set(struct rContext *C, struct ScrArea *);
void CTX_wm_region_set(struct rContext *C, struct ARegion *);
void CTX_data_main_set(struct rContext *C, struct Main *);
void CTX_data_scene_set(struct rContext *C, struct Scene *);

/**
 * Gets pointer to the dependency graph.
 * If it doesn't exist yet, it will be allocated.
 *
 * The result dependency graph is NOT guaranteed to be up-to-date neither from relation nor from
 * evaluated data points of view.
 *
 * \note Can not be used if access to a fully evaluated data-block is needed.
 */
struct Depsgraph *CTX_data_depsgraph_pointer(const struct rContext *C);

/**
 * Get dependency graph which is expected to be fully evaluated.
 *
 * In the release builds it is the same as CTX_data_depsgraph_pointer(). In the debug builds extra
 * sanity checks are done. Additionally, this provides more semantic meaning to what is exactly
 * expected to happen.
 */
struct Depsgraph *CTX_data_expect_evaluated_depsgraph(const struct rContext *C);

/**
 * Gets fully updated and evaluated dependency graph.
 *
 * All the relations and evaluated objects are guaranteed to be up to date.
 *
 * \note Will be expensive if there are relations or objects tagged for update.
 * \note If there are pending updates depsgraph hooks will be invoked.
 */
struct Depsgraph *CTX_data_ensure_evaluated_depsgraph(const struct rContext *C);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_CONTEXT_H
