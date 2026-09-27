#include "MEM_guardedalloc.h"

#include "KER_object.h"
#include "KER_layer.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "overlay_engine.h"
#include "overlay_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_engine.h"
#include "intern/draw_manager.h"

/* -------------------------------------------------------------------- */
/** \name Overlay Draw Engine Cache
 * \{ */

ROSE_STATIC void overlay_cache_init(void *vdata) {
	overlay_armature_cache_init((DRWOverlayData *)vdata);
	overlay_outline_cache_init((DRWOverlayData *)vdata);
}

ROSE_STATIC void overlay_cache_populate(void *vdata, Object *object) {
	const bool draw_outlines = (object->flag_base & BASE_SELECTED) != 0;

#define ROUTE(type, function) case type: function((DRWOverlayData *)vdata, object); break

	switch (object->type) {
		ROUTE(OB_ARMATURE, overlay_armature_cache_populate);
	}

#undef ROUTE

	if (draw_outlines) {
		overlay_outline_cache_populate(vdata, object);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Overlay Draw Engine
 * \{ */

ROSE_STATIC void overlay_init(void *vdata) {
	DRWOverlayViewportFramebufferList *fbl = ((DRWOverlayData *)vdata)->fbl;
	DRWOverlayViewportTextureList *txl = ((DRWOverlayData *)vdata)->txl;
	DRWOverlayViewportStorageList *stl = ((DRWOverlayData *)vdata)->stl;

	if (!stl->data) {
		stl->data = MEM_callocN(sizeof(DRWOverlayViewportPrivateData), "DRWOverlayViewportPrivateData");
	}

	DefaultTextureList *dtxl = DRW_view_data_texture_list_get(GDrawManager.vdata_engine, GDrawManager.viewport);

	const int size[2] = {GPU_texture_width(dtxl->depth), GPU_texture_height(dtxl->depth)};
	if ((txl->texture_depth != NULL) && ((GPU_texture_width(txl->texture_depth) != size[0]) || (GPU_texture_height(txl->texture_depth) != size[1]))) {
		GPU_TEXTURE_FREE_SAFE(txl->texture_depth);
	}
	if ((txl->texture_color != NULL) && ((GPU_texture_width(txl->texture_color) != size[0]) || (GPU_texture_height(txl->texture_color) != size[1]))) {
		GPU_TEXTURE_FREE_SAFE(txl->texture_color);
	}
	if ((txl->texture_outline_object_id != NULL) && ((GPU_texture_width(txl->texture_outline_object_id) != size[0]) || (GPU_texture_height(txl->texture_outline_object_id) != size[1]))) {
		GPU_TEXTURE_FREE_SAFE(txl->texture_outline_object_id);
	}

	if (txl->texture_color == NULL) {
		txl->texture_color = GPU_texture_create_2d("ColorTexture", size[0], size[1], 1, GPU_RGBA32F, GPU_TEXTURE_USAGE_ATTACHMENT, NULL);
	}
	if (txl->texture_depth == NULL) {
		txl->texture_depth = GPU_texture_create_2d("DepthTexture", size[0], size[1], 1, GPU_DEPTH_COMPONENT32F, GPU_TEXTURE_USAGE_ATTACHMENT, NULL);
	}
	if (txl->texture_outline_object_id == NULL) {
		txl->texture_outline_object_id = GPU_texture_create_2d("ObjectIdBuffer", size[0], size[1], 1, GPU_R32UI, GPU_TEXTURE_USAGE_ATTACHMENT, NULL);
	}

	GPU_framebuffer_ensure_config(&fbl->outline_prepass_fb,
								  {
									  GPU_ATTACHMENT_TEXTURE(dtxl->depth),
									  GPU_ATTACHMENT_TEXTURE(txl->texture_color),
									  GPU_ATTACHMENT_TEXTURE(txl->texture_outline_object_id),
								  });

	GPU_framebuffer_ensure_config(&fbl->color_only_fb,
								  {
									  GPU_ATTACHMENT_NONE,
									  GPU_ATTACHMENT_TEXTURE(dtxl->color),
								  });

	DRW_overlay_private_data_init(stl->data);
	DRW_overlay_update_world_ubo(stl->data);
}

ROSE_STATIC void overlay_draw(void *vdata) {
	DRWOverlayViewportFramebufferList *fbl = ((DRWOverlayData *)vdata)->fbl;
	DRWOverlayViewportPassList *psl = ((DRWOverlayData *)vdata)->psl;

	DefaultFramebufferList *dfbl = DRW_view_data_framebuffer_list_get(GDrawManager.vdata_engine, GDrawManager.viewport);

	DRW_draw_pass(psl->armature_pass);

	if (psl->outline_geometry_prepass) {
		GPU_framebuffer_bind(fbl->outline_prepass_fb);
		DRW_draw_pass(psl->outline_geometry_prepass);

		if (psl->outline_pass) {
			GPU_framebuffer_bind(fbl->color_only_fb);
			DRW_draw_pass(psl->outline_pass);
		}
	}
}

ROSE_STATIC void overlay_free(void) {
	DRW_overlay_shader_instance_formats_free();
	DRW_overlay_shaders_free();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Overlay Draw Engine Type Definition
 * \{ */

static const DrawEngineDataSize draw_overlay_engine_data_size = DRW_VIEWPORT_DATA_SIZE(DRWOverlayData);

DrawEngineType draw_engine_overlay_type = {
	.name = "ROSE_OVERLAY",

	.vdata_size = &draw_overlay_engine_data_size,

	.engine_init = overlay_init,

	.cache_init = overlay_cache_init,
	.cache_populate = overlay_cache_populate,
	.cache_finish = NULL,

	.draw = overlay_draw,

	.engine_free = overlay_free,
};

/** \} */
