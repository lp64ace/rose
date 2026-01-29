#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_state.h"
#include "GPU_viewport.h"

#include "KER_object.h"
#include "KER_modifier.h"

#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "basic_engine.h"
#include "basic_private.h"

#include "intern/draw_defines.h"
#include "intern/draw_manager.h"

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine Types
 * \{ */

typedef struct DRWBasicViewportPrivateData {
	DRWShadingGroup *opaque_shgroup;
} DRWBasicViewportPrivateData;

typedef struct DRWBasicViewportPassList {
	DRWPass *opaque_pass;
} DRWBasicViewportPassList;

typedef struct DRWBasicViewportStorageList {
	DRWBasicViewportPrivateData *data;
} DRWBasicViewportStorageList;

typedef struct DRWBasicData {
	struct ViewportEngineData *prev, *next;

	void *engine;
	DRWViewportEmptyList *fbl;
	DRWViewportEmptyList *txl;
	DRWBasicViewportPassList *psl;
	DRWBasicViewportStorageList *stl;
} DRWBasicData;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine Cache
 * \{ */

ROSE_STATIC void basic_cache_init(void *vdata) {
	DRWViewportEmptyList *fbl = ((DRWBasicData *)vdata)->fbl;
	DRWViewportEmptyList *txl = ((DRWBasicData *)vdata)->txl;
	DRWBasicViewportPassList *psl = ((DRWBasicData *)vdata)->psl;
	DRWBasicViewportStorageList *stl = ((DRWBasicData *)vdata)->stl;

	if (!stl->data) {
		stl->data = MEM_callocN(sizeof(DRWBasicViewportPrivateData), "DRWBasicViewportPrivateData");
	}

	DRWBasicViewportPrivateData *impl = stl->data;

	GPUShader *opaque = DRW_basic_shader_opaque_get();
	if ((psl->opaque_pass = DRW_pass_new("Opaque Pass", DRW_STATE_DEFAULT))) {
		impl->opaque_shgroup = DRW_shading_group_new(opaque, psl->opaque_pass);

		DRW_shading_group_clear_ex(impl->opaque_shgroup, GPU_DEPTH_BIT, NULL, 1.0f, 0);
	}
}

ROSE_INLINE bool basic_modifier_supported(int mdtype) {
	return ELEM(mdtype, MODIFIER_TYPE_ARMATURE);
}

ROSE_STATIC void basic_cache_populate(void *vdata, struct Object *object) {
	DRWBasicViewportStorageList *stl = ((DRWBasicData *)vdata)->stl;
	DRWBasicViewportPrivateData *impl = stl->data;

	if (!ELEM(object->type, OB_MESH)) {
		return;
	}

	GPUBatch *surface = DRW_cache_object_surface_get(object);

	bool has_defgroup_modifier = false;

	/**
	 * Since this engine is capable of handling deform modifier on the device
	 * instead of the CPU we need to create the uniform buffers for the bone
	 * (pose channel) matrices.
	 *
	 *                          &&&&&&&&&&&&&&&&&&&
	 *						 &&&&&&&&&&&&$&&&&$&:&&&&&&
	 *					   &&& & &&&& +X&$$$&&&&&;     &&
	 *					  & X:   .& : &$$$$&&&XX&&&&&&&&&&&
	 *					 &;&&&:&&&&&&&&&&&&& &&&&&&&&&&&$&&
	 *					&&&&&&&&$&&&&&&&&&&+ &;&&&&&&&&&&$&&&&
	 *					&&$&&$&&&&&&&&&& &&   +& && & &&&$&&&
	 *					&&$&X$&&&$ && &  &     ; && & &&$$&& +
	 *					&&&$&&& & &&  .  &          &  &&&&&
	 *					&&$$&& :                        && &$
	 *					 &&&&& ;   ;&&&&&&X;   &&&&&    &&
	 *						&& + &  &&        :  &&  &  &
	 *					&&+ ;& +                       & $
	 *					 &x;   :+&                    .  &
	 *					   &$&+   $           &       &&
	 *						   &&::&                 &
	 *							 &$X;     &&&&      &
	 *							   +&X            &
	 *							   + .&&       .&
	 *						  & & &    :;&&X &
	 *						&   +X: ::     :;&    :&&&&+
	 *					   +     X;X$X+   ::X    &&&& &&&.
	 *					 &       &$   $&&&& X    &      &
	 *				  &                     X    +x     :
	 *			   &              &   X&     &&&&  &;     &
	 *			&         &        &            $   &       &
	 *		  &                ; X &&&&+&&&&&&&+&:::  +;$&    .&
	 *						&&       &&&&&  &&             &    :
	 *	   +    &                + X           && & &  & &&&      &
	 *	   &        &       +            &&&$&&            & &     &
	 *	   &     ;     &    x   &                     ;;           .
	 *	   &      &     &     &          &&&&$&           &
	 *	   &       &      & +                && ;             :&    &
	 *	  .         &    ;               &&&+ &  &&:;&   &
	 *	 &           &  &              &&&&&  &         x&     &;
	 *	 &            &       $&&&&&&&$       &   x .&     $ & &     &
	 *				 &      X&&             . & X x&       &  &      &
	 *	 &          ;     $X;&             $   &&&&       :     ;
	 *			  &     &Xx$X              .         :  ;  & &        &
	 *	&    &&&&    x&+::&                              &
	 *	;$&&.X    &&$;: :&                              X
	 *	   $   &&X;     $                                             &$
	 *	   ;                                                &
	 *
	 * The only problem; What happens when the modifier is in the wrong order since this is applied last,
	 * what happens when there are mutliple deform modifiers?
	 *
	 * \note We let Sinji Ikari worry about that for now.
	 */
	LISTBASE_FOREACH(ModifierData *, md, &object->modifiers) {
		if ((md->flag & MODIFIER_DEVICE_ONLY) == 0 || !basic_modifier_supported(md->type)) {
			continue;
		}

		switch (md->type) {
			case MODIFIER_TYPE_ARMATURE: {
				GPUUniformBuf *block = DRW_basic_defgroup_ubo(object, md);

				DRW_shading_group_bind_uniform_block(impl->opaque_shgroup, block, DRW_DVGROUP_UBO_SLOT);
				has_defgroup_modifier |= (block != NULL);
			} break;
		}
	}

	if (!has_defgroup_modifier) {
		GPUUniformBuf *block = DRW_basic_defgroup_ubo(object, NULL);

		DRW_shading_group_bind_uniform_block(impl->opaque_shgroup, block, DRW_DVGROUP_UBO_SLOT);
	}

	if (impl->opaque_shgroup) {
		DRW_shading_group_call_ex(impl->opaque_shgroup, object, object->obmat, surface);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine
 * \{ */

ROSE_STATIC void basic_init(void *vdata) {
}

ROSE_STATIC void basic_draw(void *vdata) {
	DRWBasicViewportPassList *psl = ((DRWBasicData *)vdata)->psl;

	DRW_draw_pass(psl->opaque_pass);
}

ROSE_STATIC void basic_free(void) {
	DRW_basic_shaders_free();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine Type Definition
 * \{ */

static const DrawEngineDataSize draw_basic_engine_data_size = DRW_VIEWPORT_DATA_SIZE(DRWBasicData);

DrawEngineType draw_engine_basic_type = {
	.name = "ROSE_BASIC",

	.vdata_size = &draw_basic_engine_data_size,

	.engine_init = basic_init,
	.engine_free = basic_free,

	.cache_init = basic_cache_init,
	.cache_populate = basic_cache_populate,
	.cache_finish = NULL,

	.draw = basic_draw,
};

/** \} */
