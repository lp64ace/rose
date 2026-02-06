#include "MEM_guardedalloc.h"

#include "KER_object.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "overlay_engine.h"
#include "overlay_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_manager.h"

/* -------------------------------------------------------------------- */
/** \name Overlay Draw Engine Cache
 * \{ */

ROSE_STATIC void overlay_cache_init(void *vdata) {
	overlay_armature_cache_init((DRWOverlayData *)vdata);
}

ROSE_STATIC void overlay_cache_populate(void *vdata, Object *object) {
#define ROUTE(type, function) case type: function((DRWOverlayData *)vdata, object); break

	switch (object->type) {
		ROUTE(OB_ARMATURE, overlay_armature_cache_populate);
	}

#undef ROUTE
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Overlay Draw Engine
 * \{ */

ROSE_STATIC void overlay_init(void *vdata) {
	DRWOverlayData *data = (DRWOverlayData *)vdata;
	DRWOverlayViewportStorageList *stl = data->stl;

	if (!stl->data) {
		stl->data = MEM_callocN(sizeof(DRWOverlayViewportPrivateData), "DRWOverlayViewportPrivateData");
	}
}

ROSE_STATIC void overlay_draw(void *vdata) {
	DRWOverlayViewportPassList *psl = ((DRWOverlayData *)vdata)->psl;

	DRW_draw_pass(psl->armature_pass);
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
