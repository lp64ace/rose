#pragma once

#include "GPU_shader.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct GPUShader;

/**
 * Dispatch a compute shader task.
 * The number of work groups (aka thread groups) is bounded by #GPU_INFO_MAX_WORK_GROUP_COUNT_* which
 * might be different in each of the 3 dimensions.
 */
void GPU_compute_dispatch(struct GPUShader *shader, unsigned int groups_x_len, unsigned int groups_y_len, unsigned int groups_z_len);

#if defined(__cplusplus)
}
#endif
