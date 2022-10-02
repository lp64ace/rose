#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_image_shuffle_color)
    .AdditionalInfo("gpu_shader_2D_image_common")
    .PushConstant(Type::VEC4, "color")
    .PushConstant(Type::VEC4, "shuffle")
    .SetFragmentSource("gpu_shader_image_shuffle_color_frag.glsl")
    .SetDoStaticCompilation(true);
