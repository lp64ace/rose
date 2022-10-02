#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_image_rect_color)
    .VertexOut(smooth_tex_coord_interp_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "color")
    .PushConstant(Type::VEC4, "rect_icon")
    .PushConstant(Type::VEC4, "rect_geom")
    .Sampler(0, ImageType::FLOAT_2D, "image")
    .SetVertexSource("gpu_shader_2D_image_rect_vert.glsl")
    .SetFragmentSource("gpu_shader_image_color_frag.glsl")
    .SetDoStaticCompilation(true);
