#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_diag_stripes)
    .VertexIn(0, Type::VEC2, "pos")
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "color1")
    .PushConstant(Type::VEC4, "color2")
    .PushConstant(Type::INT, "size1")
    .PushConstant(Type::INT, "size2")
    .SetVertexSource("gpu_shader_2D_vert.glsl")
    .SetFragmentSource("gpu_shader_diag_stripes_frag.glsl")
    .SetDoStaticCompilation(true);
