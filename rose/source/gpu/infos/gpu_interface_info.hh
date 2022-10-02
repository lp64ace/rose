#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_INTERFACE_INFO(flat_color_iface, "").Flat(Type::VEC4, "finalColor");
GPU_SHADER_INTERFACE_INFO(no_perspective_color_iface, "").NoPerspective(Type::VEC4, "finalColor");
GPU_SHADER_INTERFACE_INFO(smooth_color_iface, "").Smooth(Type::VEC4, "finalColor");
GPU_SHADER_INTERFACE_INFO(smooth_tex_coord_interp_iface, "").Smooth(Type::VEC2, "texCoord_interp");
GPU_SHADER_INTERFACE_INFO(smooth_radii_iface, "").Smooth(Type::VEC2, "radii");
GPU_SHADER_INTERFACE_INFO(smooth_radii_outline_iface, "").Smooth(Type::VEC4, "radii");
GPU_SHADER_INTERFACE_INFO(flat_color_smooth_tex_coord_interp_iface, "")
    .Flat(Type::VEC4, "finalColor")
    .Smooth(Type::VEC2, "texCoord_interp");
