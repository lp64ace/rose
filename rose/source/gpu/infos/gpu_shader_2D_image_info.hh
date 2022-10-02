#pragma once

#include "gpu/intern/gpu_shader_create_info.h"
#include "gpu_interface_info.hh"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_image_common)
    .VertexIn(0, Type::VEC2, "pos")
    .VertexIn (1, Type::VEC2, "texCoord")
    .VertexOut(smooth_tex_coord_interp_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .Sampler(0, ImageType::FLOAT_2D, "image")
    .SetVertexSource("gpu_shader_2D_image_vert.glsl");
