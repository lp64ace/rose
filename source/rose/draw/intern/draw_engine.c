#include "MEM_guardedalloc.h"

#include "GPU_framebuffer.h"
#include "GPU_texture.h"

#include "LIB_math_vector.h"
#include "LIB_mempool.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "DRW_render.h"

#include "draw_engine.h"
#include "draw_manager.h"

DRWViewData *DRW_view_data_new(ListBase *engines) {
	DRWViewData *view_data = (DRWViewData *)MEM_callocN(sizeof(DRWViewData), "DRWViewData");
	if (view_data) {
		LISTBASE_FOREACH(DrawEngineType *, engine, engines) {
			ViewportEngineData *vdata = MEM_callocN(sizeof(ViewportEngineData), "ViewportEngineData");

			vdata->engine = engine;
			vdata->fbl = NULL;
			vdata->txl = NULL;
			vdata->psl = NULL;
			vdata->stl = NULL;

			LIB_addtail(&view_data->viewport_engine_data, vdata);
		}
	}
	return view_data;
}

ROSE_STATIC void draw_view_engine_data_clear(ViewportEngineData *vdata) {
	const DrawEngineDataSize *vdata_size = vdata->engine->vdata_size;

	for (int i = 0; vdata->fbl && i < vdata_size->fbl_len; i++) {
		GPU_FRAMEBUFFER_FREE_SAFE(vdata->fbl->framebuffers[i]);
	}
	for (int i = 0; vdata->txl && i < vdata_size->txl_len; i++) {
		GPU_TEXTURE_FREE_SAFE(vdata->txl->textures[i]);
	}
	for (int i = 0; vdata->stl && i < vdata_size->stl_len; i++) {
		MEM_SAFE_FREE(vdata->stl->storage[i]);
	}

	MEM_SAFE_FREE(vdata->fbl);
	MEM_SAFE_FREE(vdata->txl);
	MEM_SAFE_FREE(vdata->psl);
	MEM_SAFE_FREE(vdata->stl);
}

ROSE_STATIC void draw_view_data_clear(DRWViewData *view_data) {
	GPU_FRAMEBUFFER_FREE_SAFE(view_data->dfbl.default_fb);

	if ((view_data->flag & DRW_VIEW_DATA_VIEWPORT) == 0) {
		GPU_TEXTURE_FREE_SAFE(view_data->dtxl.depth);
		GPU_TEXTURE_FREE_SAFE(view_data->dtxl.color);
	}
	else {
		view_data->dtxl.depth = NULL;
		view_data->dtxl.color = NULL;
		view_data->flag &= ~DRW_VIEW_DATA_VIEWPORT;
	}

	LISTBASE_FOREACH(ViewportEngineData *, vdata, &view_data->viewport_engine_data) {
		draw_view_engine_data_clear(vdata);
	}
}

void DRW_view_data_free(DRWViewData *view_data) {
	draw_view_data_clear(view_data);

	LIB_freelistN(&view_data->viewport_engine_data);

	GPU_UNIFORMBUF_DISCARD_SAFE(GDraw.view);

	MEM_freeN(view_data);
}

void DRW_view_data_texture_list_size_validate(DRWViewData *view_data, const int size[2]) {
	if (!equals_v2_v2_int(view_data->txl_size, size)) {
		draw_view_data_clear(view_data);
		copy_v2_v2_int(view_data->txl_size, size);
	}
}

void DRW_view_data_default_lists_from_viewport(DRWViewData *view_data, GPUViewport *viewport) {
	int view = GPU_viewport_active_view_get(viewport);

	DefaultFramebufferList *dfbl = &view_data->dfbl;
	DefaultTextureList *dtxl = &view_data->dtxl;

	dtxl->depth = GPU_viewport_depth_texture(viewport);
	dtxl->color = GPU_viewport_color_texture(viewport, view);

	GPU_framebuffer_ensure_config(&dfbl->default_fb,
								  {
									  GPU_ATTACHMENT_TEXTURE(dtxl->depth),
									  GPU_ATTACHMENT_TEXTURE(dtxl->color),
								  });

	view_data->flag |= DRW_VIEW_DATA_VIEWPORT;
}

void DRW_view_data_use_engine(DRWViewData *view_data, DrawEngineType *engine_type) {
	ViewportEngineData *engine = DRW_view_data_engine_data_get_ensure(view_data, engine_type);

	UNUSED_VARS(engine);
}

ViewportEngineData *DRW_view_data_engine_data_get_ensure(DRWViewData *view_data, DrawEngineType *engine_type) {
	LISTBASE_FOREACH(ViewportEngineData *, vdata, &view_data->viewport_engine_data) {
		if (vdata->engine != engine_type) {
			continue;
		}

		const DrawEngineDataSize *data_size = engine_type->vdata_size;

		if (vdata->fbl == NULL || vdata->txl == NULL || vdata->psl == NULL || vdata->stl == NULL) {
			vdata->fbl = MEM_callocN(sizeof(DRWViewportEngineDataFramebufferList) * data_size->fbl_len, "DRWEngine::FrameBufferList");
			vdata->txl = MEM_callocN(sizeof(DRWViewportEngineDataTextureList) * data_size->txl_len, "DRWEngine::TextureList");
			vdata->psl = MEM_callocN(sizeof(DRWViewportEngineDataPassList) * data_size->psl_len, "DRWEngine::PassList");
			vdata->stl = MEM_callocN(sizeof(DRWViewportEngineDataStorageList) * data_size->stl_len, "DRWEngine::StorageList");
		}

		return vdata;
	}

	return NULL;
}
