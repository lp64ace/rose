#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_3D_uniform_color)
    .VertexIn(0, Type::VEC3, "pos")
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "color")
    .SetVertexSource("gpu_shader_3D_vert.glsl")
    .SetFragmentSource("gpu_shader_uniform_color_frag.glsl")
    .AdditionalInfo("gpu_srgb_to_framebuffer_space")
    .SetDoStaticCompilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_uniform_color_clipped)
    .AdditionalInfo("gpu_shader_3D_uniform_color")
    .AdditionalInfo("gpu_clip_planes")
    .SetDoStaticCompilation(true);

/* Confusing naming convention. But this is a version with only one local clip plane. */
GPU_SHADER_CREATE_INFO(gpu_shader_3D_clipped_uniform_color)
    .VertexIn(0, Type::VEC3, "pos")
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "color")
    /* TODO(@fclem): Put those two to one UBO. */
    .PushConstant(Type::MAT4, "ModelMatrix")
    .PushConstant(Type::VEC4, "ClipPlane")
    .SetVertexSource("gpu_shader_3D_clipped_uniform_color_vert.glsl")
    .SetFragmentSource("gpu_shader_uniform_color_frag.glsl")
    .AdditionalInfo("gpu_srgb_to_framebuffer_space")
    .SetDoStaticCompilation(true);
