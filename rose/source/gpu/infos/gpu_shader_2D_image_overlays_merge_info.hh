#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_image_overlays_merge)
    .VertexIn(0, Type::VEC2, "pos")
    .VertexIn(1, Type::VEC2, "texCoord")
    .VertexOut(smooth_tex_coord_interp_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::BOOL, "display_transform")
    .PushConstant(Type::BOOL, "overlay")
    /* Sampler slots should match OCIO's. */
    .Sampler(0, ImageType::FLOAT_2D, "image_texture")
    .Sampler(1, ImageType::FLOAT_2D, "overlays_texture")
    .SetVertexSource("gpu_shader_2D_image_vert.glsl")
    .SetFragmentSource("gpu_shader_image_overlays_merge_frag.glsl")
    .SetDoStaticCompilation(true);
