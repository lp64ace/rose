#ifndef OVERLAY_PRIVATE_H
#define OVERLAY_PRIVATE_H

#include "GPU_shader.h"
#include "GPU_uniform_buffer.h"
#include "GPU_vertex_format.h"

#include "DRW_render.h"

struct DRWPass;
struct GPUShader;
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

struct GPUShader *DRW_overlay_shader_armature_shape();
void DRW_overlay_shaders_free();

/* -------------------------------------------------------------------- */
/** \name Overlay Draw Engine Types
 * \{ */

typedef struct DRWOverlayViewportPassList {
	struct DRWPass *armature_pass;
} DRWOverlayViewportPassList;

typedef struct DRWOverlayViewportPrivateData {
	struct DRWShadingGroup *armature_shgroup;

	DRWOverlayArmatureCallBuffers armature_call_buffers;
} DRWOverlayViewportPrivateData;

typedef struct DRWOverlayViewportStorageList {
	DRWOverlayViewportPrivateData *data;
} DRWOverlayViewportStorageList;

typedef struct DRWOverlayData {
	struct ViewportEngineData *prev, *next;

	void *engine;
	DRWViewportEmptyList *fbl;
	DRWViewportEmptyList *txl;
	DRWOverlayViewportPassList *psl;
	DRWOverlayViewportStorageList *stl;
} DRWOverlayData;

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

/** \} */

#ifdef __cplusplus
}
#endif

#endif
