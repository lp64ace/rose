#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

/* We leverage hardware interpolation to compute distance along the line. */
GPU_SHADER_INTERFACE_INFO(gpu_shader_line_dashed_interface, "")
    .NoPerspective(Type::VEC2, "stipple_start") /* In screen space */
    .Flat(Type::VEC2, "stipple_pos");            /* In screen space */

GPU_SHADER_CREATE_INFO(gpu_shader_3D_line_dashed_uniform_color)
    .VertexIn(0, Type::VEC3, "pos")
    .VertexOut(flat_color_iface)
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC2, "viewport_size")
    .PushConstant(Type::FLOAT, "dash_width")
    .PushConstant(Type::FLOAT, "dash_factor") /* if > 1.0, solid line. */
    /* TODO(fclem): Remove this. And decide to discard if color2 alpha is 0. */
    .PushConstant(Type::INT, "colors_len") /* Enabled if > 0, 1 for solid line. */
    .PushConstant(Type::VEC4, "color")
    .PushConstant(Type::VEC4, "color2")
    .VertexOut(gpu_shader_line_dashed_interface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .SetVertexSource("gpu_shader_3D_line_dashed_uniform_color_vert.glsl")
    .SetFragmentSource("gpu_shader_2D_line_dashed_frag.glsl")
    .SetDoStaticCompilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_line_dashed_uniform_color_clipped)
    .PushConstant(Type::MAT4, "ModelMatrix")
    .AdditionalInfo("gpu_shader_3D_line_dashed_uniform_color")
    .AdditionalInfo("gpu_clip_planes")
    .SetDoStaticCompilation(true);
