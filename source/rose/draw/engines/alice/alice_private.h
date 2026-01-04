#ifndef ALICE_PRIVATE_H
#define ALICE_PRIVATE_H

#include "GPU_shader.h"
#include "GPU_uniform_buffer.h"

#include "DRW_render.h"

#include "KER_lib_id.h"

struct ModifierData;
struct Object;
struct GPUShader;
struct GPUUniformBuf;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AliceDrawData {
    DrawData dd;

    /** The uniform buffer used to deform the bones of a mesh, see #alice_modifier.c */
    GPUUniformBuf *defgroup;
} AliceDrawData;

/**
 * Returns the shader for the depth pass, builds the shader if not already built.
 * Call #DRW_alice_shaders_free to free all the loaded shaders!
 */
GPUShader *DRW_alice_shader_opaque_get(void);

void DRW_alice_shaders_free();

/**
 * Returns the uniform buffer that should be used to deform the vertex group using 
 * the specified modifier data.
 */
GPUUniformBuf *DRW_alice_defgroup_ubo(struct Object *object, struct ModifierData *md);

#ifdef __cplusplus
}
#endif

#endif
