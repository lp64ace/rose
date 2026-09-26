#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "workbench_engine.h"
#include "workbench_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_engine.h"
#include "intern/draw_manager.h"

void DRW_workbench_outline_cache_init(DRWWorkbenchData *vdata) {
	DRWWorkbenchViewportTextureList *txl = vdata->txl;
	DRWWorkbenchViewportPassList *psl = vdata->psl;
	DRWWorkbenchViewportStorageList *stl = vdata->stl;
	DRWWorkbenchViewportPrivateData *impl = stl->data;

	GPUShader *outline = DRW_workbench_shader_outline_get();

	if (!(psl->outline_pass = DRW_pass_new("Outline", DRW_STATE_WRITE_COLOR | DRW_STATE_BLEND_ALPHA_PREMUL))) {
		return;
	}

	DefaultTextureList *dtxl = DRW_view_data_texture_list_get(GDrawManager.vdata_engine, GDrawManager.viewport);

	impl->outline_shgroup = DRW_shading_group_new(outline, psl->outline_pass);
	DRW_shading_group_uniform_texture(impl->outline_shgroup, "objectIdBuffer", txl->texture_object_id);
	DRW_shading_group_uniform_texture(impl->outline_shgroup, "depthBuffer", dtxl->depth);
	DRW_shading_group_uniform_block(impl->outline_shgroup, "worldData", impl->world_ubo);
	DRW_shading_group_call_procedural_triangles(impl->outline_shgroup, NULL, 1);
}