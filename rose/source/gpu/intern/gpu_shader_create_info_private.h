#pragma once

#include "gpu/gpu_shader.h"

namespace rose {
namespace gpu {

void gpu_shader_create_info_init ( void );
void gpu_shader_create_info_exit ( void );

bool gpu_shader_create_info_compile_all ( void );

const GPU_ShaderCreateInfo *gpu_shader_create_info_get ( const char *info_name );

}
}
