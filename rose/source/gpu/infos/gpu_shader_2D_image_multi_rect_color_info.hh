#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_image_multi_rect_color)
    .VertexIn(0, Type::VEC2, "pos")
    .VertexOut(flat_color_smooth_tex_coord_interp_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .UniformBuf(0, "MultiRectCallData", "multi_rect_data")
    .Sampler(0, ImageType::FLOAT_2D, "image")
    .SetTypedefSource("GPU_shader_shared.h")
    .SetVertexSource("gpu_shader_2D_image_multi_rect_vert.glsl")
    .SetFragmentSource("gpu_shader_image_varying_color_frag.glsl")
    .SetDoStaticCompilation(true);
