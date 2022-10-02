#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_instance_varying_color_varying_size)
    .VertexIn(0, Type::VEC3, "pos")
    .VertexIn(1, Type::VEC4, "color")
    .VertexIn(2, Type::FLOAT, "size")
    .VertexIn(3, Type::MAT4, "InstanceModelMatrix")
    .VertexOut(flat_color_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ViewProjectionMatrix")
    .SetVertexSource("gpu_shader_instance_variying_size_variying_color_vert.glsl")
    .SetFragmentSource("gpu_shader_flat_color_frag.glsl")
    .AdditionalInfo("gpu_srgb_to_framebuffer_space")
    .SetDoStaticCompilation(true);
