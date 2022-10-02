#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_point_uniform_size_uniform_color_outline_aa)
    .VertexIn(0, Type::VEC2, "pos")
    .VertexOut(smooth_radii_outline_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "color")
    .PushConstant(Type::VEC4, "outlineColor")
    .PushConstant(Type::FLOAT, "size")
    .PushConstant(Type::FLOAT, "outlineWidth")
    .SetVertexSource("gpu_shader_2D_point_uniform_size_outline_aa_vert.glsl")
    .SetFragmentSource("gpu_shader_point_uniform_color_outline_aa_frag.glsl")
    .SetDoStaticCompilation(true);
