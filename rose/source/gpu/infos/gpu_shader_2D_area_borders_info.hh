#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_INTERFACE_INFO(smooth_uv_iface, "").Smooth(Type::VEC2, "uv");

GPU_SHADER_CREATE_INFO(gpu_shader_2D_area_borders)
    .VertexIn(0, Type::VEC2, "pos")
    .VertexOut(smooth_uv_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "rect")
    .PushConstant(Type::VEC4, "color")
    .PushConstant(Type::FLOAT, "scale")
    .PushConstant(Type::INT, "cornerLen")
    .SetVertexSource("gpu_shader_2D_area_borders_vert.glsl")
    .SetFragmentSource("gpu_shader_2D_area_borders_frag.glsl")
    .SetDoStaticCompilation(true);
