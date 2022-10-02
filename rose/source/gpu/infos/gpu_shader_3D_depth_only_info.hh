#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_3D_depth_only)
    .VertexIn(0, Type::VEC3, "pos")
    .VertexOut(flat_color_iface)
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .SetVertexSource("gpu_shader_3D_vert.glsl")
    .SetFragmentSource("gpu_shader_depth_only_frag.glsl")
    .SetDoStaticCompilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_depth_only_clipped)
    .AdditionalInfo("gpu_shader_3D_depth_only")
    .AdditionalInfo("gpu_clip_planes")
    .SetDoStaticCompilation(true);
