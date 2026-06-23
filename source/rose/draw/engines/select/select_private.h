#ifndef SELECT_PRIVATE_H
#define SELECT_PRIVATE_H

#include "GPU_shader.h"

#ifdef __cplusplus
extern "C" {
#endif

struct GPUShader *DRW_select_shader_id_flat_get(void);
struct GPUShader *DRW_select_shader_id_uniform_get(void);

void DRW_select_shaders_free(void);

#ifdef __cplusplus
}
#endif

#endif
