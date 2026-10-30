#ifndef KER_CONTEXT_H
#define KER_CONTEXT_H

#include "LIB_listbase.h"

struct ID;
struct StructRNA;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rContext rContext;
typedef struct rContextDataResult rContextDataResult;

typedef enum eContextResult {
	/* The context member was found, and its data is available. */
	CTX_RESULT_OK = 1,

	/* The context member was not found. */
	CTX_RESULT_MEMBER_NOT_FOUND = 0,

	/**
	 * The context member was found, but its data is not available.
	 * For example, "active_bone" is a valid context member, but has not data in Object mode.
	 */
	CTX_RESULT_NO_DATA = -1,
} eContextResult;

/* Function mapping a context member name to its value. */
typedef int (*fnContextDataCallback)(const rContext *C, const char *member, struct rContextDataResult *result);

void CTX_data_id_list_add(struct rContextDataResult *result, struct ID *id);
void CTX_data_list_add(struct rContextDataResult *result, struct ID *id, struct StructRNA *type, void *data);

int ctx_data_list_count(const struct rContext *C, int (*func)(const struct rContext *, ListBase *));

#define CTX_DATA_COUNT(C, member) ctx_data_list_count(C, CTX_data_##member)

/**
 * Data Context
 *
 * - listbases consist of CollectionPointerLink items and must be
 *   freed with LIB_freelistN!
 * - the dir listbase consists of LinkData items
 */

/* data type, needed so we can tell between a NULL pointer and an empty list */
typedef enum eContextDataType {
	CTX_DATA_TYPE_POINTER = 0,
	CTX_DATA_TYPE_COLLECTION,
} eContextDataType;

bool CTX_data_dir(const char *member);
void CTX_data_dir_set(struct rContextDataResult *result, const char **dir);

void CTX_data_type_set(struct rContextDataResult *result, eContextDataType type);
eContextDataType CTX_data_type_get(struct rContextDataResult *result);

#define CTX_DATA_BEGIN(C, Type, instance, member) \
	{ \
		ListBase ctx_data_list; \
		CollectionPointerLink *ctx_link; \
		CTX_data_##member(C, &ctx_data_list); \
		for (ctx_link = (CollectionPointerLink *)ctx_data_list.first; ctx_link; ctx_link = ctx_link->next) { \
			Type instance = (Type)ctx_link->ptr.data;

#define CTΧ_DATA_END \
		} \
		LIB_freelistN(&ctx_data_list); \
	} \
	(void)0

int CTX_data_selected_objects(const rContext *C, ListBase *list);
int CTX_data_selected_bases(const rContext *C, ListBase *list);

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
struct ReportList;
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
struct ReportList *CTX_wm_reports(const struct rContext *C);
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

struct SpaceLink;
struct SpaceFile;

struct SpaceLink *CTX_wm_space_data(const struct rContext *C);
struct SpaceFile *CTX_wm_space_file(const struct rContext *C);
struct View3D *CTX_wm_space_view3d(const struct rContext *C);

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
