#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_3D_smooth_color)
    .VertexIn(0, Type::VEC3, "pos")
    .VertexIn(1, Type::VEC4, "color")
    .VertexOut(smooth_color_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .SetVertexSource("gpu_shader_3D_smooth_color_vert.glsl")
    .SetFragmentSource("gpu_shader_3D_smooth_color_frag.glsl")
    .AdditionalInfo("gpu_srgb_to_framebuffer_space")
    .SetDoStaticCompilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_smooth_color_clipped)
    .AdditionalInfo("gpu_shader_3D_smooth_color")
    .AdditionalInfo("gpu_clip_planes")
    .SetDoStaticCompilation(true);
