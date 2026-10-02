#ifndef WORKBENCH_PRIVATE_H
#define WORKBENCH_PRIVATE_H

#include "DNA_object_types.h"

#include "GPU_framebuffer.h"
#include "GPU_shader.h"
#include "GPU_texture.h"
#include "GPU_uniform_buffer.h"

#include "DRW_render.h"

#include "KER_lib_id.h"

#include "intern/shaders/draw_shader_shared.h"

struct DRWShadingGroup;
struct ModifierData;
struct Object;
struct GPUFrameBuffer;
struct GPUShader;
struct GPUTexture;
struct GPUUniformBuf;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Workbench Draw Data Types
 * \{ */

typedef struct WorkbenchDrawData {
	DrawData dd;

	/* Shadow direction in local object space. */
	float shadow_dir[3];
	float shadow_depth;
	/* Min, max in shadow space */
	float shadow_min[3];
	float shadow_max[3];

	BoundBox shadow_box;

	int flag;
} WorkbenchDrawData;

typedef struct WorkbenchViewLayerDrawData {
	/** All constant data used for a render loop. */
	struct GPUUniformBuf *world_ubo;
} WorkbenchViewLayerDrawData;

/** #WorkbenchDrawData->flag */
enum {
	WORKBENCH_SHADOW_BOX_DIRTY = 1 << 0,
};

/** !!!This needs to be aligned to 16 for Uniform Buffer usage!!! */
typedef struct WorkbenchWorldUBO {
	float viewport_size[2], viewport_size_inv[2];

	float object_outline_color[4];
	float shadow_direction_vs[4];
	float shadow_direction_ws[4];

	float shadow_shift;
	float shadow_focus;
	float shadow_mul;
	float shadow_add;
} WorkbenchWorldUBO;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Workbench Draw Data Routines
 * \{ */

/**
 * Returns the shader for the depth pass, builds the shader if not already built.
 * Call #DRW_workbench_shaders_free to free all the loaded shaders!
 */
struct GPUShader *DRW_workbench_shader_depth_get(void);
struct GPUShader *DRW_workbench_shader_opaque_get(void);
struct GPUShader *DRW_workbench_shader_shadow_pass_get(bool manifold);
struct GPUShader *DRW_workbench_shader_shadow_fail_get(bool manifold, bool cap);
struct GPUShader *DRW_workbench_shader_outline_get(void);

void DRW_workbench_shaders_free();

/**
 * Returns the uniform buffer that should be used to deform the vertex group using
 * the specified modifier data.
 */
struct GPUUniformBuf *DRW_workbench_defgroup_ubo(struct Object *object, struct ModifierData *md);
struct WorkbenchDrawData *DRW_workbench_drawdata(struct Object *object);
struct WorkbenchDrawData *DRW_workbench_view_layer_drawdata(struct Object *object);

void DRW_workbench_modifier_list_build(struct DRWShadingGroup *shgroup, struct Object *object);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Workbench Draw Engine Types
 * \{ */

typedef struct DRWWorkbenchViewportFramebufferList {
	struct GPUFrameBuffer *color_only_fb;
	struct GPUFrameBuffer *opaque_fb;
} DRWWorkbenchViewportFramebufferList;

typedef struct DRWWorkbenchViewportTextureList {
	struct GPUTexture *texture_object_id;
} DRWWorkbenchViewportTextureList;

typedef struct DRWWorkbenchViewportPrivateData {
	/* World */

	GPUUniformBuf *world_ubo;

	/* Shadow */

	/** Previous shadow direction to test if shadow has changed. */
	float shadow_cached_direction[3];
	/** Current shadow direction in world space. */
	float shadow_direction_ws[3];
	/** Shadow precomputed matrices. */
	float shadow_mat[4][4];
	float shadow_inv[4][4];
	/** Far plane of the view frustum. Used for shadow volume extrusion. */
	float shadow_far_plane[4];
	/** Min and max of shadow_near_corners. Speed up culling test. */
	float shadow_near_min[3];
	float shadow_near_max[3];
	/** This is a parallelogram, so only 2 normal and distance to the edges. */
	float shadow_near_sides[2][4];
	/* Shadow shading groups. First array elem is for non-manifold geom and second for manifold. */
	struct DRWShadingGroup *shadow_pass_shgroup[2];
	struct DRWShadingGroup *shadow_fail_shgroup[2];
	struct DRWShadingGroup *shadow_caps_shgroup[2];
	/** If the shadow has changed direction and ob bboxes needs to be updated. */
	bool shadow_changed;

	/* Opaque */
	
	struct DRWShadingGroup *depth_shgroup;
	struct DRWShadingGroup *opaque_shgroup[2];
	struct DRWShadingGroup *outline_shgroup;
} DRWWorkbenchViewportPrivateData;

typedef struct DRWWorkbenchViewportPassList {
	struct DRWPass *depth_pass;
	struct DRWPass *shadow_pass[2];
	struct DRWPass *opaque_pass[2];
	struct DRWPass *outline_pass;
} DRWWorkbenchViewportPassList;

typedef struct DRWWorkbenchViewportStorageList {
	struct DRWWorkbenchViewportPrivateData *data;
} DRWWorkbenchViewportStorageList;

typedef struct DRWWorkbenchData {
	struct ViewportEngineData *prev, *next;

	int flag;

	void *engine;
	struct DRWWorkbenchViewportFramebufferList *fbl;
	struct DRWWorkbenchViewportTextureList *txl;
	struct DRWWorkbenchViewportPassList *psl;
	struct DRWWorkbenchViewportStorageList *stl;
} DRWWorkbenchData;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Workbench Draw Engine Routines
 * \{ */

void DRW_workbench_shadow_cache_init(struct DRWWorkbenchData *vdata);
void DRW_workbench_shadow_cache_populate(struct DRWWorkbenchData *vdata, struct Object *object);
void DRW_workbench_shadow_cache_finish(struct DRWWorkbenchData *vdata);

void DRW_workbench_opaque_cache_init(struct DRWWorkbenchData *vdata);
void DRW_workbench_opaque_cache_populate(struct DRWWorkbenchData *vdata, struct Object *object);
void DRW_workbench_opaque_cache_finish(struct DRWWorkbenchData *vdata);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Workbench Draw Engine Data
 * \{ */

void DRW_workbench_private_data_init(struct DRWWorkbenchViewportPrivateData *impl);
void DRW_workbench_update_world_ubo(struct DRWWorkbenchViewportPrivateData *impl);
void DRW_workbench_shadow_data_update(struct DRWWorkbenchViewportPrivateData *impl, struct WorkbenchWorldUBO *data);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Workbench Draw Engine Effects
 * \{ */

void DRW_workbench_outline_cache_init(struct DRWWorkbenchData *vdata);

/** \} */

#ifdef __cplusplus
}
#endif

#endif
