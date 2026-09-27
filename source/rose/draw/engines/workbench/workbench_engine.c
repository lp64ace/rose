#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "KER_armature.h"
#include "KER_layer.h"
#include "KER_object.h"
#include "KER_modifier.h"

#include "LIB_assert.h"
#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "workbench_engine.h"
#include "workbench_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_engine.h"
#include "intern/draw_manager.h"

/* -------------------------------------------------------------------- */
/** \name Workbench Draw Engine Cache
 * \{ */

ROSE_STATIC void workbench_cache_init(void *vdata) {
	DRW_workbench_shadow_cache_init((DRWWorkbenchData *)vdata);
	DRW_workbench_opaque_cache_init((DRWWorkbenchData *)vdata);
	DRW_workbench_outline_cache_init((DRWWorkbenchData *)vdata);
}

ROSE_STATIC void workbench_cache_populate(void *vdata, Object *object) {
	WorkbenchDrawData *wdd = (WorkbenchDrawData *)KER_drawdata_get(&object->id, &draw_engine_workbench_type);

	DRW_workbench_shadow_cache_populate((DRWWorkbenchData *)vdata, object);
	DRW_workbench_opaque_cache_populate((DRWWorkbenchData *)vdata, object);
}

ROSE_STATIC void workbench_cache_finish(void *vdata) {
	DRW_workbench_shadow_cache_finish((DRWWorkbenchData *)vdata);
	DRW_workbench_opaque_cache_finish((DRWWorkbenchData *)vdata);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Workbench Draw Engine
 * \{ */

ROSE_STATIC void workbench_init(void *vdata) {
	DRWWorkbenchViewportFramebufferList *fbl = ((DRWWorkbenchData *)vdata)->fbl;
	DRWWorkbenchViewportTextureList *txl = ((DRWWorkbenchData *)vdata)->txl;
	DRWWorkbenchViewportStorageList *stl = ((DRWWorkbenchData *)vdata)->stl;
		
	if (!stl->data) {
		stl->data = MEM_callocN(sizeof(DRWWorkbenchViewportPrivateData), "DRWWorkbenchViewportPrivateData");
	}

	DRWWorkbenchViewportPrivateData *impl = stl->data;

	DefaultTextureList *dtxl = DRW_view_data_texture_list_get(GDrawManager.vdata_engine, GDrawManager.viewport);

	const int size[2] = {GPU_texture_width(dtxl->depth), GPU_texture_height(dtxl->depth)};
	if ((txl->texture_object_id != NULL) && ((GPU_texture_width(txl->texture_object_id) != size[0]) || (GPU_texture_height(txl->texture_object_id) != size[1]))) {
		GPU_TEXTURE_FREE_SAFE(txl->texture_object_id);
	}

	if (txl->texture_object_id == NULL) {
		txl->texture_object_id = GPU_texture_create_2d("ObjectIdBuffer", size[0], size[1], 1, GPU_R32UI, GPU_TEXTURE_USAGE_ATTACHMENT, NULL);
	}

	const unsigned int empty = 0u;
	GPU_texture_clear(txl->texture_object_id, GPU_DATA_UINT, (const void *)&empty);

	GPU_framebuffer_ensure_config(&fbl->opaque_fb,
								  {
									  GPU_ATTACHMENT_TEXTURE(dtxl->depth),
									  GPU_ATTACHMENT_TEXTURE(dtxl->color),
									  GPU_ATTACHMENT_TEXTURE(txl->texture_object_id),
								  });

	GPU_framebuffer_ensure_config(&fbl->color_only_fb,
								  {
									  GPU_ATTACHMENT_NONE,
									  GPU_ATTACHMENT_TEXTURE(dtxl->color),
								  });

	DRW_workbench_private_data_init(impl);
	DRW_workbench_update_world_ubo(impl);
}

ROSE_STATIC void workbench_draw(void *vdata) {
	DRWWorkbenchViewportPassList *psl = ((DRWWorkbenchData *)vdata)->psl;

	DRW_draw_pass(psl->depth_pass);
	DRW_draw_pass(psl->shadow_pass[0]);
	DRW_draw_pass(psl->shadow_pass[1]);
	DRW_draw_pass(psl->opaque_pass[0]);
	DRW_draw_pass(psl->opaque_pass[1]);
}

ROSE_STATIC void workbench_free(void) {
	DRW_workbench_shaders_free();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Workbench Draw Engine Type Definition
 * \{ */

static const DrawEngineDataSize draw_workbench_engine_data_size = DRW_VIEWPORT_DATA_SIZE(DRWWorkbenchData);

DrawEngineType draw_engine_workbench_type = {
	.name = "ROSE_WORKBENCH",

	.vdata_size = &draw_workbench_engine_data_size,

	.engine_init = workbench_init,

	.cache_init = workbench_cache_init,
	.cache_populate = workbench_cache_populate,
	.cache_finish = workbench_cache_finish,

	.draw = workbench_draw,

	.engine_free = workbench_free,
};

/** \} */
