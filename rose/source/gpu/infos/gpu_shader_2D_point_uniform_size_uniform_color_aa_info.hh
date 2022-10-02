#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_point_uniform_size_uniform_color_aa)
    .VertexIn(0, Type::VEC2, "pos")
    .VertexOut(smooth_radii_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "color")
    .PushConstant(Type::FLOAT, "size")
    .SetVertexSource("gpu_shader_2D_point_uniform_size_aa_vert.glsl")
    .SetFragmentSource("gpu_shader_point_uniform_color_aa_frag.glsl")
    .SetDoStaticCompilation(true);
