#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_point_varying_size_varying_color)
    .VertexIn(0, Type::VEC2, "pos")
    .VertexIn(1, Type::FLOAT, "size")
    .VertexIn(2, Type::VEC4, "color")
    .VertexOut(smooth_color_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .SetVertexSource("gpu_shader_2D_point_varying_size_varying_color_vert.glsl")
    .SetFragmentSource("gpu_shader_point_varying_color_frag.glsl")
    .SetDoStaticCompilation(true);
