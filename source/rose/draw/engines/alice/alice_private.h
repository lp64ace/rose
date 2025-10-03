#ifndef ALICE_PRIVATE_H
#define ALICE_PRIVATE_H

#include "DRW_render.h"

struct GPUShader;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the shader for the depth pass, builds the shader if not already built.
 * Call #DRW_alice_shaders_free to free all the loaded shaders!
 */
GPUShader *DRW_alice_shader_depth_get(void);

void DRW_alice_shaders_free();

#ifdef __cplusplus
}
#endif

#endif
