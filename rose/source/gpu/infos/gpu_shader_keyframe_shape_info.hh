#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_INTERFACE_INFO(keyframe_shape_iface, "")
    .Flat(Type::VEC4, "finalColor")
    .Flat(Type::VEC4, "finalOutlineColor")
    .Flat(Type::VEC4, "radii")
    .Flat(Type::VEC4, "thresholds")
    .Flat(Type::INT, "finalFlags");

GPU_SHADER_CREATE_INFO(gpu_shader_keyframe_shape)
    .VertexIn(0, Type::VEC4, "color")
    .VertexIn(1, Type::VEC4, "outlineColor")
    .VertexIn(2, Type::VEC2, "pos")
    .VertexIn(3, Type::FLOAT, "size")
    .VertexIn(4, Type ::INT, "flags")
    .VertexOut(keyframe_shape_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC2, "ViewportSize")
    .PushConstant(Type::FLOAT, "outline_scale")
    .SetVertexSource("gpu_shader_keyframe_shape_vert.glsl")
    .SetFragmentSource("gpu_shader_keyframe_shape_frag.glsl")
    .SetDoStaticCompilation(true);
