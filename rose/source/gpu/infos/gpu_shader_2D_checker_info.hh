#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_checker)
    .VertexIn(0, Type::VEC2, "pos")
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "color1")
    .PushConstant(Type::VEC4, "color2")
    .PushConstant(Type::INT, "size")
    .SetVertexSource("gpu_shader_2D_vert.glsl")
    .SetFragmentSource("gpu_shader_checker_frag.glsl")
    .SetDoStaticCompilation(true);
