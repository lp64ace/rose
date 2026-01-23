#ifndef BASIC_PRIVATE_H
#define BASIC_PRIVATE_H

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

typedef struct BasicDrawData {
	DrawData dd;

	/** The uniform buffer used to deform the bones of a mesh, see #basic_modifier.c */
	GPUUniformBuf *defgroup;
} BasicDrawData;

/**
 * Returns the shader for the depth pass, builds the shader if not already built.
 * Call #DRW_basic_shaders_free to free all the loaded shaders!
 */
GPUShader *DRW_basic_shader_opaque_get(void);
GPUShader *DRW_basic_shader_normal_get(void);

void DRW_basic_shaders_free();

/**
 * Returns the uniform buffer that should be used to deform the vertex group using
 * the specified modifier data.
 */
GPUUniformBuf *DRW_basic_defgroup_ubo(struct Object *object, struct ModifierData *md);

#ifdef __cplusplus
}
#endif

#endif
