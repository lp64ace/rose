#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_INTERFACE_INFO(smooth_normal_iface, "").Smooth(Type::VEC3, "normal");

GPU_SHADER_CREATE_INFO(gpu_shader_simple_lighting)
    .VertexIn(0, Type::VEC3, "pos")
    .VertexIn(1, Type::VEC3, "nor")
    .VertexOut(smooth_normal_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .UniformBuf(0, "SimpleLightingData", "simple_lighting_data", GPU_FREQUENCY_PASS)
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::MAT3, "NormalMatrix")
    .SetTypedefSource("GPU_shader_shared.h")
    .SetVertexSource("gpu_shader_3D_normal_vert.glsl")
    .SetFragmentSource("gpu_shader_simple_lighting_frag.glsl")
    .SetDoStaticCompilation(true);
