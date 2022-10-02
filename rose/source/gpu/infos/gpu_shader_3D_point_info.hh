#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_3D_point_fixed_size_varying_color)
    .VertexIn(0, Type::VEC3, "pos")
    .VertexIn(1, Type::VEC4, "color")
    .VertexOut(smooth_color_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .SetVertexSource("gpu_shader_3D_point_fixed_size_varying_color_vert.glsl")
    .SetFragmentSource("gpu_shader_point_varying_color_frag.glsl")
    .SetDoStaticCompilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_point_varying_size_varying_color)
    .VertexIn(0, Type::VEC3, "pos")
    .VertexIn(1, Type::VEC4, "color")
    .VertexIn(2, Type::FLOAT, "size")
    .VertexOut(smooth_color_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .SetVertexSource("gpu_shader_3D_point_varying_size_varying_color_vert.glsl")
    .SetFragmentSource("gpu_shader_point_varying_color_frag.glsl")
    .SetDoStaticCompilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_point_uniform_size_uniform_color_aa)
    .VertexIn(0, Type::VEC3, "pos")
    .VertexOut(smooth_radii_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "color")
    .PushConstant(Type::FLOAT, "size")
    .SetVertexSource("gpu_shader_3D_point_uniform_size_aa_vert.glsl")
    .SetFragmentSource("gpu_shader_point_uniform_color_aa_frag.glsl")
    .SetDoStaticCompilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_point_uniform_size_uniform_color_aa_clipped)
    .AdditionalInfo("gpu_shader_3D_point_uniform_size_uniform_color_aa")
    .AdditionalInfo("gpu_clip_planes")
    .SetDoStaticCompilation(true);
