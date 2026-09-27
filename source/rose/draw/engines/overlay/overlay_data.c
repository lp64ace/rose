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
#include "KER_modifier.h"
#include "KER_object.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
#include "LIB_utildefines.h"

#include "overlay_engine.h"
#include "overlay_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_engine.h"
#include "intern/draw_manager.h"

ROSE_STATIC void overlay_view_layer_data_free(void *storage) {
	OverlayViewLayerDrawData *vldata = (OverlayViewLayerDrawData *)storage;

	GPU_UNIFORMBUF_DISCARD_SAFE(vldata->world_ubo);
}

ROSE_STATIC OverlayViewLayerDrawData *overlay_view_layer_data_ensure_ex(ViewLayer *view_layer) {
	OverlayViewLayerDrawData **vldata = (OverlayViewLayerDrawData **)DRW_view_layer_engine_data_ensure_ex(view_layer, &draw_engine_overlay_type, &overlay_view_layer_data_free);
	if (*vldata == NULL) {
		*vldata = (OverlayViewLayerDrawData *)MEM_callocN(sizeof(**vldata), "OverlayViewLayerDrawData");
		(*vldata)->world_ubo = GPU_uniformbuf_create_ex(sizeof(OverlayWorldUBO), NULL, "OverlayWorldUBO");
	}

	return *vldata;
}

void DRW_overlay_private_data_init(DRWOverlayViewportPrivateData *impl) {
	OverlayViewLayerDrawData *vldata = overlay_view_layer_data_ensure_ex(GDrawManager.view_layer);

	impl->world_ubo = vldata->world_ubo;
}

void DRW_overlay_update_world_ubo(DRWOverlayViewportPrivateData *impl) {
	OverlayWorldUBO data;

	copy_v4_fl4(data.object_outline_color, 0.9f, 0.5f, 0.2f, 1.0f);
	copy_v2_v2(data.viewport_size, DRW_viewport_size_get());
	copy_v2_v2(data.viewport_size_inv, DRW_viewport_invert_size_get());

	GPU_uniformbuf_update(impl->world_ubo, &data);
}
