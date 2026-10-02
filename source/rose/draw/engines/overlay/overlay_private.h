#ifndef OVERLAY_PRIVATE_H
#define OVERLAY_PRIVATE_H

#include "GPU_framebuffer.h"
#include "GPU_shader.h"
#include "GPU_texture.h"
#include "GPU_uniform_buffer.h"
#include "GPU_vertex_format.h"

#include "DRW_render.h"

struct DRWPass;
struct GPUFrameBuffer;
struct GPUShader;
struct GPUTexture;
struct Object;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DRWOverlayInstanceFormats {
	struct GPUVertFormat *instance_bone;
} DRWOverlayInstanceFormats;

typedef struct DRWOverlayArmatureCallBuffersInner {
	struct DRWCallBuffer *octahaedron;
} DRWOverlayArmatureCallBuffersInner;

typedef struct DRWOverlayArmatureCallBuffers {
	DRWOverlayArmatureCallBuffersInner solid;
	DRWOverlayArmatureCallBuffersInner transparent;
} DRWOverlayArmatureCallBuffers;

struct DRWOverlayInstanceFormats *DRW_overlay_shader_instance_formats();
void DRW_overlay_shader_instance_formats_free();

struct GPUShader *DRW_overlay_shader_geometry_prepass_get();
struct GPUShader *DRW_overlay_shader_outline_get();
struct GPUShader *DRW_overlay_shader_armature_shape();
void DRW_overlay_shaders_free();

/* -------------------------------------------------------------------- */
/** \name Overlay Draw Engine Types
 * \{ */

typedef struct DRWOverlayViewportFramebufferList {
	struct GPUFrameBuffer *color_only_fb;
	struct GPUFrameBuffer *outline_prepass_fb;
} DRWOverlayViewportFramebufferList;

typedef struct DRWOverlayViewportTextureList {
	struct GPUTexture *texture_depth;
	struct GPUTexture *texture_color;
	struct GPUTexture *texture_outline_object_id;
} DRWOverlayViewportTextureList;

typedef struct DRWOverlayViewportPassList {
	struct DRWPass *armature_pass;
	struct DRWPass *outline_geometry_prepass;
	struct DRWPass *outline_pass;
} DRWOverlayViewportPassList;

typedef struct DRWOverlayViewportPrivateData {
	struct DRWShadingGroup *armature_shgroup;
	struct DRWShadingGroup *outline_prepass_shgroup;
	struct DRWShadingGroup *outline_shgroup;

	struct GPUUniformBuf *world_ubo;

	DRWOverlayArmatureCallBuffers armature_call_buffers;
} DRWOverlayViewportPrivateData;

typedef struct DRWOverlayViewportStorageList {
	DRWOverlayViewportPrivateData *data;
} DRWOverlayViewportStorageList;

typedef struct DRWOverlayData {
	struct ViewportEngineData *prev, *next;

	int flag;

	void *engine;
	DRWOverlayViewportFramebufferList *fbl;
	DRWOverlayViewportTextureList *txl;
	DRWOverlayViewportPassList *psl;
	DRWOverlayViewportStorageList *stl;
} DRWOverlayData;

typedef struct OverlayViewLayerDrawData {
	/** All constant data used for a render loop. */
	struct GPUUniformBuf *world_ubo;
} OverlayViewLayerDrawData;

/** !!!This needs to be aligned to 16 for Uniform Buffer usage!!! */
typedef struct OverlayWorldUBO {
	float viewport_size[2], viewport_size_inv[2];

	float object_outline_color[4];
} OverlayWorldUBO;

/** armature */

typedef struct BoneInstanceData {
	/* Keep sync with bone instance vertex format (DRWOverlayInstanceFormats) */
	union {
		float mat[4][4];
		struct {
			float _pad0[3], color_hint_a;
			float _pad1[3], color_hint_b;
			float _pad2[3], color_a;
			float _pad3[3], color_b;
		};
		struct {
			float _pad00[3], amin_a;
			float _pad01[3], amin_b;
			float _pad02[3], amax_a;
			float _pad03[3], amax_b;
		};
	};
} BoneInstanceData;

void overlay_armature_cache_init(struct DRWOverlayData *vdata);
void overlay_armature_cache_populate(struct DRWOverlayData *vdata, struct Object *object);
void overlay_outline_cache_init(struct DRWOverlayData *vdata);
void overlay_outline_cache_populate(struct DRWOverlayData *vdata, struct Object *object);

void DRW_overlay_private_data_init(struct DRWOverlayViewportPrivateData *impl);
void DRW_overlay_update_world_ubo(struct DRWOverlayViewportPrivateData *impl);

/** \} */

#ifdef __cplusplus
}
#endif

#endif
