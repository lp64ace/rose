#include "MEM_guardedalloc.h"

#include "RNA_access.h"
#include "RNA_prototypes.h"

#include "KER_context.h"
#include "KER_object.h"
#include "KER_layer.h"
#include "KER_scene.h"
#include "KER_screen.h"

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
		int recursion;
	} data;
} rContext;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Shared API
 * \{ */

struct rContextDataResult {
	PointerRNA ptr;
	ListBase list;
	const char **dir;
	eContextDataType type;
};

void CTX_data_id_list_add(rContextDataResult *result, struct ID *id) {
	CollectionPointerLink *link = MEM_callocN(sizeof(CollectionPointerLink), "CTX_data_id_list_add");
	link->ptr = RNA_id_pointer_create(id);

	LIB_addtail(&result->list, link);
}

void CTX_data_list_add(rContextDataResult *result, ID *id, StructRNA *type, void *data) {
	CollectionPointerLink *link = MEM_callocN(sizeof(CollectionPointerLink), "CTX_data_list_add");
	link->ptr = RNA_pointer_create_discrete(id, type, data);

	LIB_addtail(&result->list, link);
}

int ctx_data_list_count(const rContext *C, int (*func)(const rContext *, ListBase *)) {
	ListBase list;

	if (func(C, &list)) {
		size_t total = LIB_listbase_count(&list);
		LIB_freelistN(&list);
		return total;
	}

	return 0;
}

bool CTX_data_dir(const char *member) {
	return member[0] == '\0';
}

void CTX_data_dir_set(rContextDataResult *result, const char **dir) {
	result->dir = dir;
}

void CTX_data_type_set(rContextDataResult *result, eContextDataType type) {
	result->type = type;
}

eContextDataType CTX_data_type_get(rContextDataResult *result) {
	return result->type;
}

ROSE_INLINE eContextResult ctx_data_get(rContext *C, const char *member, rContextDataResult *result) {
	Screen *screen;
	ScrArea *area;
	ARegion *region;

	eContextResult ret = CTX_RESULT_MEMBER_NOT_FOUND;

	int done = 0;
	int recursion = C->data.recursion;

	memset(result, 0, sizeof(rContextDataResult));

	if (!LIB_thread_is_main()) {
		return done;
	}

	/**
	 * we check recursion to ensure that we do not get infinite
     * loops requesting data from ourselves in a context callback
	 */
	if (done != 1 && recursion < 1 && (region = CTX_wm_region(C))) {
		C->data.recursion = 1;

		if (region->type && region->type->context) {
			ret = region->type->context(C, member, result);
			if (ret) {
				done = -(-ret | -done);
			}
		}
	}
	if (done != 1 && recursion < 2 && (area = CTX_wm_area(C))) {
		C->data.recursion = 2;

		if (area->type && area->type->context) {
			ret = area->type->context(C, member, result);
			if (ret) {
				done = -(-ret | -done);
			}
		}
	}
	if (done != 1 && recursion < 3 && (screen = CTX_wm_screen(C))) {
		C->data.recursion = 3;

		if (screen->context) {
			fnContextDataCallback cb = (fnContextDataCallback)screen->context;
			ret = cb(C, member, result);
			if (ret) {
				done = -(-ret | -done);
			}
		}
	}

	C->data.recursion = recursion;

	return done;
} 

ROSE_INLINE int ctx_data_collection_get(const rContext *C, const char *member, ListBase *list) {
	rContextDataResult result;
	if (ctx_data_get((rContext *)C, member, &result) == CTX_RESULT_OK) {
    	ROSE_assert(result.type == CTX_DATA_TYPE_COLLECTION);
		*list = result.list;
		return 1;
	}

	LIB_listbase_clear(list);

	return 0;
}

ROSE_INLINE int ctx_data_base_collection_get(const rContext *C, const char *member, ListBase *list) {
	ListBase ctx_object_list;
	if ((ctx_data_collection_get(C, member, &ctx_object_list) == false) || LIB_listbase_is_empty(&ctx_object_list)) {
		LIB_listbase_clear(list);
		return 0;
	}

	rContextDataResult result;
	memset(&result, 0, sizeof(rContextDataResult));

	Scene *scene = CTX_data_scene(C);
	ViewLayer *view_layer = CTX_data_view_layer(C);

	bool ok = false;

	CollectionPointerLink *ctx_object;
	for (ctx_object = ctx_object_list.first; ctx_object; ctx_object = ctx_object->next) {
		Object *ob = ctx_object->ptr.data;
		Base *base = KER_view_layer_base_find(view_layer, ob);
		if (base != NULL) {
			CTX_data_list_add(&result, &scene->id, &RNA_ObjectBase, base);
			ok = true;
		}
	}
	CTX_data_type_set(&result, CTX_DATA_TYPE_COLLECTION);
	LIB_freelistN(&ctx_object_list);

	*list = result.list;
	return ok;
}

int CTX_data_selected_objects(const rContext *C, ListBase *list) {
	return ctx_data_collection_get(C, "selected_objects", list);
}

int CTX_data_selected_bases(const rContext *C, ListBase *list) {
	return ctx_data_base_collection_get(C, "selected_objects", list);
}

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

struct ReportList *CTX_wm_reports(const rContext *C) {
	if (C->wm.manager) {
		return &C->wm.manager->runtime->reports;
	}

	return NULL;
}

struct Main *CTX_data_main(const rContext *ctx) {
	return ctx->data.main;
}

struct Scene *CTX_data_scene(const rContext *ctx) {
	return ctx->data.scene;
}

struct ViewLayer *CTX_data_view_layer(const rContext *C) {
	Scene *scene = CTX_data_scene(C);
	if (!scene) {
		return NULL;
	}

	wmWindow *window = CTX_wm_window(C);
	if (window) {
		ViewLayer *view_layer = NULL;
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

struct SpaceLink *CTX_wm_space_data(const rContext *C) {
	ScrArea *area = CTX_wm_area(C);
	if (area) {
		return (struct SpaceLink *)(area->spacedata.first);
	}
	return NULL;
}

struct SpaceFile *CTX_wm_space_file(const rContext *C) {
	ScrArea *area = CTX_wm_area(C);
	if (area && area->spacetype == SPACE_FILE) {
		return (struct SpaceFile *)(area->spacedata.first);
	}
	return NULL;
}

struct View3D *CTX_wm_space_view3d(const rContext *C) {
	ScrArea *area = CTX_wm_area(C);
	if (area && area->spacetype == SPACE_VIEW3D) {
		return (struct View3D *)(area->spacedata.first);
	}
	return NULL;
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
