#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_framebuffer.h"

#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"

#include "KER_armature.h"
#include "KER_object.h"

#include "overlay_engine.h"
#include "overlay_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_engine.h"
#include "intern/draw_manager.h"

void overlay_outline_cache_init(struct DRWOverlayData *vdata) {
	DRWOverlayViewportTextureList *txl = vdata->txl;
	DRWOverlayViewportPassList *psl = vdata->psl;
	DRWOverlayViewportStorageList *stl = vdata->stl;
	DRWOverlayViewportPrivateData *impl = stl->data;

	GPUShader *geometry_prepass = DRW_overlay_shader_geometry_prepass_get();

	if (true /* do overlay outline prepass */) {
		if (!(psl->outline_geometry_prepass = DRW_pass_new("Outline Prepass", DRW_STATE_DEFAULT))) {
			return;
		}

		const unsigned int empty = 0u;
		GPU_texture_clear(txl->texture_outline_object_id, GPU_DATA_UINT, (const void *)&empty);

		impl->outline_prepass_shgroup = DRW_shading_group_new(geometry_prepass, psl->outline_geometry_prepass);
		DRW_shading_group_clear_ex(impl->outline_prepass_shgroup, GPU_DEPTH_BIT, NULL, 1.0f, 0x00);
		DRW_shading_group_clear_ex(impl->outline_prepass_shgroup, GPU_COLOR_BIT, NULL, 1.0f, 0x00);
	}
	else {
		psl->outline_geometry_prepass = NULL;
	}

	GPUShader *outline = DRW_overlay_shader_outline_get();

	if (true /* do overlay outline */) {
		if (!(psl->outline_pass = DRW_pass_new("Outline", DRW_STATE_WRITE_COLOR | DRW_STATE_BLEND_ALPHA_PREMUL))) {
			return;
		}

		const unsigned int empty = 0u;
		GPU_texture_clear(txl->texture_outline_object_id, GPU_DATA_UINT, (const void *)&empty);

		impl->outline_shgroup = DRW_shading_group_new(outline, psl->outline_pass);
		DRW_shading_group_uniform_texture(impl->outline_shgroup, "objectIdBuffer", txl->texture_outline_object_id);
		DRW_shading_group_uniform_block(impl->outline_shgroup, "worldData", impl->world_ubo);
		DRW_shading_group_call_procedural_triangles(impl->outline_shgroup, NULL, 1);
	}
	else {
		psl->outline_pass = NULL;
	}
}

ROSE_INLINE void overlay_outline_cache_populate_mesh(struct DRWOverlayData *vdata, struct Object *object) {
	DRWOverlayViewportStorageList *stl = (vdata)->stl;
	DRWOverlayViewportPrivateData *impl = stl->data;

	GPUBatch *surface = DRW_cache_object_surface_get(object);

	DRW_shading_group_call_ex(impl->outline_prepass_shgroup, object, object->obmat, surface);
}

void overlay_outline_cache_populate(struct DRWOverlayData *vdata, struct Object *object) {
	switch (object->type) {
		case OB_MESH:
			overlay_outline_cache_populate_mesh(vdata, object);
			break;
	}
}
