#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"
#include "GPU_uniform_buffer.h"
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

ROSE_STATIC void workbench_view_layer_data_free(void *storage) {
	WorkbenchViewLayerDrawData *vldata = (WorkbenchViewLayerDrawData *)storage;

	GPU_UNIFORMBUF_DISCARD_SAFE(vldata->world_ubo);
}

ROSE_STATIC WorkbenchViewLayerDrawData *workbench_view_layer_data_ensure_ex(ViewLayer *view_layer) {
#define UNIQUE_ENGINE_PTR (DrawEngineType *)&workbench_view_layer_data_ensure_ex

	WorkbenchViewLayerDrawData **vldata = (WorkbenchViewLayerDrawData **)DRW_view_layer_engine_data_ensure_ex(view_layer, UNIQUE_ENGINE_PTR, &workbench_view_layer_data_free);
	if (*vldata == NULL) {
		*vldata = (WorkbenchViewLayerDrawData *)MEM_callocN(sizeof(**vldata), "WorkbenchViewLayerDrawData");
		(*vldata)->world_ubo = GPU_uniformbuf_create_ex(sizeof(WorkbenchWorldUBO), NULL, "WorkbenchWorldUBO");
	}

	return *vldata;

#undef UNIQUE_ENGINE_PTR
}

void DRW_workbench_private_data_init(DRWWorkbenchViewportPrivateData *impl) {
	WorkbenchViewLayerDrawData *vldata = workbench_view_layer_data_ensure_ex(GDrawManager.view_layer);

	impl->world_ubo = vldata->world_ubo;
}

void DRW_workbench_update_world_ubo(DRWWorkbenchViewportPrivateData *impl) {
	WorkbenchWorldUBO data;

	copy_v4_fl4(data.object_outline_color, 1.0f, 0.5f, 0.25f, 1.0f);
	copy_v2_v2(data.viewport_size, DRW_viewport_size_get());
	copy_v2_v2(data.viewport_size_inv, DRW_viewport_invert_size_get());

	DRW_workbench_shadow_data_update(impl, &data);

	GPU_uniformbuf_update(impl->world_ubo, &data);
}