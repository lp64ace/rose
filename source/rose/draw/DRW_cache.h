#ifndef DRW_CACHE_H
#define DRW_CACHE_H

struct GPUBatch;
struct GPUUniformBuf;

struct Object;
struct Mesh;
struct Scene;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MeshBatchCache MeshBatchCache;

/* -------------------------------------------------------------------- */
/** \name Predefined cache objects
 * \{ */

struct GPUBatch *DRW_cache_fullscreen_quad_get(void);

void DRW_global_cache_free(void);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Default cache objects
 * \{ */

const struct Object *DRW_batch_cache_device_armature(const struct Object *object);

/**
 * This should be called before or during cache population of an engine!
 * This shall queue the build for the requested batch and shall be ready upon cache finish!
 */
struct GPUBatch *DRW_cache_object_surface_get(struct Object *object);

/** Ensure that the buffer and draw batches are alloacted */
void DRW_batch_cache_validate(struct Object *object);
/** Ensure that the buffer and draw batches are initialized (not filled) */
void DRW_batch_cache_generate(struct Object *object);

// Mesh

void DRW_mesh_batch_cache_free(struct Mesh *mesh);
/** Called directly from Kernel, see the enums KER_MESH_BATCH_DIRTY_* */
void DRW_mesh_batch_cache_tag_dirty(struct Mesh *mesh, int mode);

/** \} */

#ifdef __cplusplus
}
#endif

#endif
