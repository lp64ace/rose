#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_srgb_to_framebuffer_space)
    .PushConstant(Type::BOOL, "srgbTarget")
    .Define("blender_srgb_to_framebuffer_space(a) a");
