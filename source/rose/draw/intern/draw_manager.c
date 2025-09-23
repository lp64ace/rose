#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "DNA_ID_enums.h"
#include "DNA_userdef_types.h"

#include "KER_context.h"
#include "KER_main.h"
#include "KER_mesh.h"

#include "GPU_batch.h"
#include "GPU_matrix.h"
#include "GPU_framebuffer.h"
#include "GPU_viewport.h"
#include "GPU_state.h"

#include "LIB_listbase.h"
#include "LIB_mempool.h"

#include "WM_draw.h"

#include "engines/alice/alice_engine.h"
#include "engines/basic/basic_engine.h"

struct Object;

/* -------------------------------------------------------------------- */
/** \name Draw Engines
 * \{ */

static ListBase GEngineList;

void DRW_engines_register(void) {
	DRW_engine_register(&draw_engine_basic_type);
	DRW_engine_register(&draw_engine_alice_type);

	{
		KER_mesh_batch_cache_tag_dirty_cb = DRW_mesh_batch_cache_tag_dirty;
		KER_mesh_batch_cache_free_cb = DRW_mesh_batch_cache_free;
	}
}

void DRW_engines_register_experimental(void) {
}

void DRW_engines_free(void) {
	LISTBASE_FOREACH_MUTABLE(DrawEngineType *, engine, &GEngineList) {
		
	}
	LIB_listbase_clear(&GEngineList);
}

void DRW_engine_register(DrawEngineType *draw_engine_type) {
	LIB_addtail(&GEngineList, draw_engine_type);
}

DrawEngineType *DRW_engine_find(const char *name) {
	DrawEngineType *engine = LIB_findstr(&GEngineList, name, offsetof(DrawEngineType, name));
	if (!engine) {
		engine = LIB_findstr(&GEngineList, "ROSE_BASIC", offsetof(DrawEngineType, name));
	}
	return engine;
}

DrawEngineType *DRW_engine_type(const struct rContext *C, struct Scene *scene) {
	return DRW_engine_find(U.engine);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Data Internal
 * \{ */

typedef struct IDDrawDataTemplate {
	ID id;
	DrawDataList drawdata;
} IDDrawDataTemplate;

ROSE_STATIC bool id_type_can_have_drawdata(const short type) {
	return ELEM(type, ID_OB);
}

ROSE_STATIC DrawDataList *drw_drawdatalist(struct ID *id) {
	if (id_type_can_have_drawdata(GS(id->name))) {
		IDDrawDataTemplate *template = (IDDrawDataTemplate *)id;
		return &template->drawdata;
	}
	return NULL;
}

ROSE_STATIC DrawData *drw_drawdata(struct ID *id, DrawEngineType *engine) {
	DrawDataList *drawdata = drw_drawdatalist(id);
	if (drawdata) {
		LISTBASE_FOREACH(DrawData *, data, drawdata) {
			if (data->engine == engine) {
				return data;
			}
		}
	}
	return NULL;
}

ROSE_STATIC DrawData *drw_drawdata_ensure(struct ID *id, DrawEngineType *engine, size_t size, DrawDataInitCb init, DrawDataFreeCb free) {
	ROSE_assert(size >= sizeof(DrawData));
	ROSE_assert(id_type_can_have_drawdata(GS(id->name)));

	DrawData *data = drw_drawdata(id, engine);
	if (data != NULL) {
		return data;
	}

	DrawDataList *list = drw_drawdatalist(id);

	data = MEM_callocN(size, "DrawData");
	data->engine = engine;
	data->free = free;

	if (init != NULL) {
		init(data);
	}

	LIB_addtail(list, data);

	return data;
}

ROSE_STATIC void drw_drawdata_free(struct ID *id) {
	DrawDataList *list = drw_drawdatalist(id);

	if (list != NULL) {
		LISTBASE_FOREACH(DrawData *, data, list) {
			if (data->free) {
				data->free(data);
			}
		}

		LIB_freelistN(list);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Engines Internal
 * \{ */

typedef struct DRWData {
	struct MemPool *framebuffers;
	struct MemPool *passes;
} DRWData;

typedef struct DRWManager {
	struct DRWData *vdata;
	struct DrawEngineType *engine;
	struct Scene *scene;
} DRWManager;

static DRWManager GDrawManager;

// Called once before starting rendering using our (used/active) draw engines
void DRW_engines_init(const struct rContext *C) {
	struct Main *main = CTX_data_main(C);
	struct ListBase *listbase = which_libbase(main, ID_SCE);

	/**
	 * User the user-defined engine preference!
	 */
	GDrawManager.engine = DRW_engine_find(U.engine);

	struct DrawEngineType *engine = GDrawManager.engine;
	struct Scene *scene = GDrawManager.scene;

	// We should be using the active scene, there is no reason whatsoever for the listbase to be a singleton!
	ROSE_assert(LIB_listbase_is_single(listbase) || LIB_listbase_is_empty(listbase));
	LISTBASE_FOREACH(struct Scene *, scene, listbase) {
		engine = DRW_engine_type(C, scene);
	}

	if (GDrawManager.engine == NULL || GDrawManager.engine != engine || GDrawManager.scene != scene) {
		if (GDrawManager.engine) {
			/**
			 * We need to REDO the cache for the new engine...
			 */
		}
		GDrawManager.engine = engine;
		GDrawManager.scene = scene;
	}
}

void DRW_engines_exit(const struct rContext *C) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw
 * Draw Away!
 * \{ */

ROSE_STATIC void drw_engine_cache_init(void) {
	if (GDrawManager.engine && GDrawManager.engine->cache_init) {
		GDrawManager.engine->cache_init(GDrawManager.vdata);
	}
}

ROSE_STATIC void drw_engine_cache_populate(struct Object *object) {
	DRW_batch_cache_validate(object);

	if (GDrawManager.engine && GDrawManager.engine->cache_populate) {
		GDrawManager.engine->cache_populate(GDrawManager.vdata, object);
	}

	// @TODO assign a different thread to generate these!
	DRW_batch_cache_generate(object);
}

ROSE_STATIC void drw_engine_cache_finish(void) {
	// @TODO wait for any threads that have beed dispatched by #drw_engine_cache_populate

	if (GDrawManager.engine && GDrawManager.engine->cache_finish) {
		GDrawManager.engine->cache_finish(GDrawManager.vdata);
	}
}

ROSE_STATIC void drw_engine_draw_scene(void) {
	if (!GDrawManager.engine || !GDrawManager.engine->draw) {
		return;
	}

	GDrawManager.engine->draw(GDrawManager.vdata);
}

void DRW_draw_render_loop(const struct rContext *C, struct ARegion *region, struct GPUViewport *viewport) {
	/** No framebuffer is allowed to be bound the moment we are rendering! */
	ROSE_assert(GPU_framebuffer_active_get() == GPU_framebuffer_back_get());

	drw_engine_cache_init();

	// We really ough to make a Dependency Graph to iterate the objects in order!
	ListBase *listbase = which_libbase(CTX_data_main(C), ID_OB);
	LISTBASE_FOREACH(struct Object *, object, listbase) {
		drw_engine_cache_populate(object);
	}

	drw_engine_cache_finish();
	drw_engine_draw_scene();
}

void DRW_draw_view(const struct rContext *C) {
	struct ARegion *region = CTX_wm_region(C);

	if (!region) {
		/** Where the fuck are we supposed to render this anyway! (make assert?) */
		return;
	}

	struct GPUViewport *viewport = WM_draw_region_get_bound_viewport(region);

	if (!viewport) {
		/** Where the fuck are we supposed to render this anyway! (make assert?) */
		return;
	}

	struct WindowManager *wm = CTX_wm_manager(C);
	struct wmWindow *win = CTX_wm_window(C);

	wm_window_make_drawable(wm, NULL);

	DRW_engines_init(C);
	DRW_draw_render_loop(C, region, viewport);
	DRW_engines_exit(C);

	wm_window_make_drawable(wm, win);
}

/** \} */
