#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_3D_image_common)
    .VertexIn(0, Type::VEC3, "pos")
    .VertexIn(1, Type::VEC2, "texCoord")
    .VertexOut(smooth_tex_coord_interp_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .Sampler(0, ImageType::FLOAT_2D, "image")
    .SetVertexSource("gpu_shader_3D_image_vert.glsl");

GPU_SHADER_CREATE_INFO(gpu_shader_3D_image)
    .AdditionalInfo("gpu_shader_3D_image_common")
    .SetFragmentSource("gpu_shader_image_frag.glsl")
    .SetDoStaticCompilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_image_color)
    .AdditionalInfo("gpu_shader_3D_image_common")
    .PushConstant(Type::VEC4, "color")
    .SetFragmentSource("gpu_shader_image_color_frag.glsl")
    .SetDoStaticCompilation(true);
