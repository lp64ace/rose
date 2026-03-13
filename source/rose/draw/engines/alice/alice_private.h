#ifndef ALICE_PRIVATE_H
#define ALICE_PRIVATE_H

#include "DNA_object_types.h"

#include "GPU_shader.h"
#include "GPU_uniform_buffer.h"

#include "DRW_render.h"

#include "KER_lib_id.h"

struct DRWShadingGroup;
struct ModifierData;
struct Object;
struct GPUShader;
struct GPUUniformBuf;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Alice Draw Data Types
 * \{ */

typedef struct AliceDrawData {
	DrawData dd;

	/* Shadow direction in local object space. */
	float shadow_dir[3];
	float shadow_depth;
	/* Min, max in shadow space */
	float shadow_min[3];
	float shadow_max[3];

	BoundBox shadow_box;

	int flag;

	/** The uniform buffer used to deform the bones of a mesh, see #alice_modifier.c */
	GPUUniformBuf *defgroup;
} AliceDrawData;

/** #AliceDrawData->flag */
enum {
	ALICE_SHADOW_BOX_DIRTY = 1 << 0,
};

/** This needs to be aligned to 16 for Uniform Buffer usage */
typedef struct AliceWorldUBO {
	float shadow_direction_vs[4];

	float shadow_shift;
	float shadow_focus;
	float shadow_mul;
	float shadow_add;
} AliceWorldUBO;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Alice Draw Data Routines
 * \{ */

/**
 * Returns the shader for the depth pass, builds the shader if not already built.
 * Call #DRW_alice_shaders_free to free all the loaded shaders!
 */
struct GPUShader *DRW_alice_shader_opaque_get(void);
struct GPUShader *DRW_alice_shader_shadow_pass_get(bool manifold);
struct GPUShader *DRW_alice_shader_shadow_fail_get(bool manifold, bool cap);

void DRW_alice_shaders_free();

/**
 * Returns the uniform buffer that should be used to deform the vertex group using
 * the specified modifier data.
 */
struct GPUUniformBuf *DRW_alice_defgroup_ubo(struct Object *object, struct ModifierData *md);
struct AliceDrawData *DRW_alice_drawdata(struct Object *object);

void DRW_alice_modifier_list_build(struct DRWShadingGroup *shgroup, struct Object *object);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Types
 * \{ */

typedef struct DRWAliceViewportPrivateData {
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

} DRWAliceViewportPrivateData;

typedef struct DRWAliceViewportPassList {
	struct DRWPass *depth_pass;
	struct DRWPass *shadow_pass[2];
	struct DRWPass *opaque_pass[2];
} DRWAliceViewportPassList;

typedef struct DRWAliceViewportStorageList {
	struct DRWAliceViewportPrivateData *data;
} DRWAliceViewportStorageList;

typedef struct DRWAliceData {
	struct ViewportEngineData *prev, *next;

	void *engine;
	DRWViewportEmptyList *fbl;
	DRWViewportEmptyList *txl;
	struct DRWAliceViewportPassList *psl;
	struct DRWAliceViewportStorageList *stl;
} DRWAliceData;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Routines
 * \{ */

void DRW_alice_shadow_cache_init(struct DRWAliceData *vdata);
void DRW_alice_shadow_cache_populate(struct DRWAliceData *vdata, struct Object *object);
void DRW_alice_shadow_cache_finish(struct DRWAliceData *vdata);

void DRW_alice_opaque_cache_init(struct DRWAliceData *vdata);
void DRW_alice_opaque_cache_populate(struct DRWAliceData *vdata, struct Object *object);
void DRW_alice_opaque_cache_finish(struct DRWAliceData *vdata);

/** \} */

#ifdef __cplusplus
}
#endif

#endif
