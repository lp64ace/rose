#pragma once

#include "LIB_sys_types.h"

#include "GPU_shader.h"

#if defined(__cplusplus)
extern "C" {
#endif

void gpu_shader_create_info_init();
void gpu_shader_create_info_exit();

bool gpu_shader_create_info_compile(const char *name_start_with_filter);

/** Runtime create infos are not registered in the dictionary and cannot be searched. */
const GPUShaderCreateInfo *gpu_shader_create_info_get(const char *info_name);

#if defined(__cplusplus)
}
#endif
