#pragma once

#include "gpu_shader.h"
// #include "gpu_storage_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

	void GPU_compute_dispatch ( GPU_Shader *shader , unsigned int groups_x_len , unsigned int groups_y_len , unsigned int groups_z_len );

#ifdef __cplusplus
}
#endif
