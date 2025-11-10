#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "DNA_ID_enums.h"
#include "DNA_userdef_types.h"

#include "KER_context.h"
#include "KER_main.h"
#include "KER_mesh.h"
#include "KER_object.h"

#include "GPU_batch.h"
#include "GPU_context.h"
#include "GPU_matrix.h"
#include "GPU_framebuffer.h"
#include "GPU_viewport.h"
#include "GPU_state.h"

#include "LIB_listbase.h"
#include "LIB_mempool.h"
#include "LIB_string.h"

#include "WM_api.h"
#include "WM_draw.h"

#include "draw_engine.h"
#include "draw_manager.h"

#include "engines/alice/alice_engine.h"
#include "engines/basic/basic_engine.h"

struct Object;

/* -------------------------------------------------------------------- */
/** \name Draw Engines
 * \{ */

ListBase GEngineList;
DRWManager GDrawManager;

void DRW_engines_register(void) {
	// DRW_engine_register(&draw_engine_basic_type);
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
		if (engine->engine_free) {
			engine->engine_free();
		}
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

	LIB_addtail((ListBase *)list, data);

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

		LIB_freelistN((ListBase *)list);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Engines Internal
 * \{ */

DRWData *DRW_viewport_data_new(void) {
	DRWData *ddata = MEM_callocN(sizeof(DRWData), "DRWData");

	ddata->commands = LIB_memory_pool_create(sizeof(DRWCommand), DRW_RESOURCE_CHUNK_LEN, 0, ROSE_MEMPOOL_NOP);
	ddata->passes = LIB_memory_pool_create(sizeof(DRWPass), DRW_RESOURCE_CHUNK_LEN, 0, ROSE_MEMPOOL_NOP);
	ddata->shgroups = LIB_memory_pool_create(sizeof(DRWShadingGroup), DRW_RESOURCE_CHUNK_LEN, 0, ROSE_MEMPOOL_NOP);
	ddata->uniforms = LIB_memory_pool_create(sizeof(DRWUniform), DRW_RESOURCE_CHUNK_LEN, 0, ROSE_MEMPOOL_NOP);

	// Resource Data Pool(s)

	ddata->obmats = LIB_memory_block_create_ex(sizeof(DRWObjectMatrix), sizeof(DRWObjectMatrix[DRW_RESOURCE_CHUNK_LEN]));

	for (size_t index = 0; index < sizeof(ddata->vdata_engine) / sizeof(ddata->vdata_engine[0]); index++) {
		ddata->vdata_engine[index] = DRW_view_data_new(&GEngineList);
	}

	return ddata;
}

void DRW_viewport_data_free(DRWData *ddata) {
	LIB_memory_pool_destroy(ddata->commands);
	LIB_memory_pool_destroy(ddata->passes);
	LIB_memory_pool_destroy(ddata->shgroups);
	LIB_memory_pool_destroy(ddata->uniforms);

	LIB_memory_block_destroy(ddata->obmats, NULL);

	for (size_t index = 0; index < sizeof(ddata->vdata_engine) / sizeof(ddata->vdata_engine[0]); index++) {
		DRW_view_data_free(ddata->vdata_engine[index]);
	}

	if (ddata->matrices_ubo != NULL) {
		for (int i = 0; i < ddata->ubo_length; i++) {
			GPU_uniformbuf_free(ddata->matrices_ubo[i]);
		}
		MEM_freeN(ddata->matrices_ubo);
	}

	MEM_freeN(ddata);
}

ROSE_STATIC void DRW_engine_use(DrawEngineType *engine_type) {
	DRW_view_data_use_engine(GDrawManager.vdata_engine, engine_type);
}

// Called once before starting rendering using our (used/active) draw engines
void DRW_engines_init(const struct rContext *C) {
	LISTBASE_FOREACH(ViewportEngineData *, vdata, &GDrawManager.vdata_engine->viewport_engine_data) {
		const DrawEngineDataSize *vdata_size = vdata->engine->vdata_size;

		memset(vdata->psl->passes, 0, sizeof(*vdata->psl->passes) * vdata_size->psl_len);

		if (vdata->engine->engine_init) {
			vdata->engine->engine_init(vdata);
		}
	}
}

void DRW_engines_exit(const struct rContext *C) {
}

DRWPass *DRW_pass_new_ex(const char *name, DRWPass *original, int state) {
	DRWPass *npass = LIB_memory_pool_calloc(GDrawManager.vdata_pool->passes);

	npass->groups.first = NULL;
	npass->groups.last = NULL;

	npass->original = original;
	npass->state = state;

	LIB_strcpy(npass->name, ARRAY_SIZE(npass->name), name);

	return npass;
}

DRWPass *DRW_pass_new(const char *name, int state) {
	return DRW_pass_new_ex(name, NULL, state);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Manager
 * \{ */

ROSE_STATIC DRWData *draw_viewport_data_ensure(GPUViewport *viewport) {
	DRWData **pvmempool = GPU_viewport_data_get(viewport);
	DRWData *vmempool = *pvmempool;

	if (vmempool == NULL) {
		*pvmempool = vmempool = DRW_viewport_data_new();
	}
	return vmempool;
}

ROSE_STATIC void draw_viewport_data_reset(DRWData *ddata) {
	LIB_memory_pool_clear(ddata->commands, DRW_RESOURCE_CHUNK_LEN);
	LIB_memory_pool_clear(ddata->passes, DRW_RESOURCE_CHUNK_LEN);
	LIB_memory_pool_clear(ddata->shgroups, DRW_RESOURCE_CHUNK_LEN);
	LIB_memory_pool_clear(ddata->uniforms, DRW_RESOURCE_CHUNK_LEN);

	LIB_memory_block_clear(ddata->obmats, NULL);
}

void DRW_manager_init(DRWManager *manager, GPUViewport *viewport, const int size[2]) {
	manager->viewport = viewport;

	int view = 0;

	if (viewport) {
		manager->vdata_pool = draw_viewport_data_ensure(viewport);
		view = GPU_viewport_active_view_get(viewport);
	}
	else {
		manager->vdata_pool = DRW_viewport_data_new();
	}

	draw_viewport_data_reset(manager->vdata_pool);

	if (viewport) {
		GPUTexture *texture = GPU_viewport_color_texture(viewport, view);
		manager->size[0] = GPU_texture_width(texture);
		manager->size[1] = GPU_texture_height(texture);
	}
	else {
		manager->size[0] = 1.0f;
		manager->size[1] = 1.0f;
	}

	manager->vdata_engine = manager->vdata_pool->vdata_engine[view];

	DRW_view_data_texture_list_size_validate(manager->vdata_engine, (const int[2]){manager->size[0], manager->size[1]});

	if (viewport) {
		DRW_view_data_default_lists_from_viewport(manager->vdata_engine, viewport);
	}

	// We should enable the needed engines!
	LISTBASE_FOREACH(ViewportEngineData *, vdata, &manager->vdata_engine->viewport_engine_data) {
		DRW_engine_use(vdata->engine);
	}

	GDrawManager.resource_handle = 0;
}

void DRW_manager_exit(DRWManager *manager) {
	if (manager->viewport) {
		// The data are persistent within the viewport, see the #GPU_viewport_free function!
	}
	else {
		DRW_viewport_data_free(manager->vdata_pool);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Init/Exit
 * \{ */

void DRW_render_context_create(struct WindowManager *wm) {
	ROSE_assert(GDrawManager.render == NULL);

	GDrawManager.mutex = LIB_mutex_alloc();
	GDrawManager.render = WM_render_context_create(wm);
	GDrawManager.context = GPU_context_create(NULL, GDrawManager.render);
	
	wm_window_reset_drawable(wm);
}

void DRW_render_context_destroy(struct WindowManager *wm) {
	if (GDrawManager.render != NULL) {
		WM_render_context_activate(GDrawManager.render);
		GPU_context_active_set(GDrawManager.context);
		GPU_context_discard(GDrawManager.context);
		WM_render_context_destroy(wm, GDrawManager.render);
		LIB_mutex_free(GDrawManager.mutex);
	}
}

void DRW_render_context_enable() {
	DRW_render_context_enable_ex(false);
}

void DRW_render_context_disable() {
	DRW_render_context_disable_ex(false);
}

void DRW_render_context_enable_ex(bool restore) {
	if (GDrawManager.render != NULL) {
		LIB_mutex_lock(GDrawManager.mutex);
		WM_render_context_activate(GDrawManager.render);
		GPU_context_active_set(GDrawManager.context);
	}
}

void DRW_render_context_disable_ex(bool restore) {
	if (GDrawManager.render != NULL) {
		GPU_context_active_set(NULL);
		WM_render_context_release(GDrawManager.render);
		LIB_mutex_unlock(GDrawManager.mutex);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw
 * Draw Away!
 * \{ */

ROSE_STATIC void drw_engine_cache_init(void) {
	LISTBASE_FOREACH(ViewportEngineData *, vdata, &GDrawManager.vdata_engine->viewport_engine_data) {
		if (vdata->engine && vdata->engine->cache_init) {
			vdata->engine->cache_init(vdata);
		}
	}
}

ROSE_STATIC void drw_engine_cache_populate(struct Object *object) {
	GDrawManager.objcache_handle = 0;

	DRW_batch_cache_validate(object);

	LISTBASE_FOREACH(ViewportEngineData *, vdata, &GDrawManager.vdata_engine->viewport_engine_data) {
		if (vdata->engine && vdata->engine->cache_populate) {
			vdata->engine->cache_populate(vdata, object);
		}
	}

	// @TODO assign a different thread to generate these!
	DRW_batch_cache_generate(object);
}

ROSE_STATIC void drw_engine_cache_finish(void) {
	// @TODO wait for any threads that have beed dispatched by #drw_engine_cache_populate

	LISTBASE_FOREACH(ViewportEngineData *, vdata, &GDrawManager.vdata_engine->viewport_engine_data) {
		if (vdata->engine && vdata->engine->cache_finish) {
			vdata->engine->cache_finish(vdata);
		}
	}

	DRW_render_buffer_finish();
}

ROSE_STATIC void drw_engine_draw_scene(void) {
	DefaultFramebufferList *dfbl = &GDrawManager.vdata_engine->dfbl;

	GPU_framebuffer_bind(dfbl->default_fb);

	LISTBASE_FOREACH(ViewportEngineData *, vdata, &GDrawManager.vdata_engine->viewport_engine_data) {
		if (vdata->engine && vdata->engine->draw) {
			vdata->engine->draw(vdata);
		}
	}

	GPU_framebuffer_restore();
}

void DRW_draw_render_loop(const struct rContext *C, struct ARegion *region, struct GPUViewport *viewport) {
	/** No framebuffer is allowed to be bound the moment we are rendering! */
	ROSE_assert(GPU_framebuffer_active_get() == GPU_framebuffer_back_get());

	DRW_manager_init(&GDrawManager, viewport, NULL);
	DRW_engines_init(C);

	ListBase *listbase = which_libbase(CTX_data_main(C), ID_OB);

	/**
	 * Definetely not the fucking place to update the mesh here,
	 * we should evaluate the depsgraph instead.
	 */
	LISTBASE_FOREACH(struct Object *, object, listbase) {
		if (object->type == OB_MESH) {
			// KER_mesh_data_update(NULL, object);
		}
	}

	drw_engine_cache_init();

	// We really ough to make a Dependency Graph to iterate the objects in order!
	LISTBASE_FOREACH(struct Object *, object, listbase) {
		drw_engine_cache_populate(object);
	}

	DRW_engines_exit(C);
	DRW_manager_exit(&GDrawManager);

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

	DRW_render_context_enable();
	DRW_draw_render_loop(C, region, viewport);
	DRW_render_context_disable();

	wm_window_reset_drawable(wm);
}

/** \} */
