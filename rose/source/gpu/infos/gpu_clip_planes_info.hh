#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_clip_planes)
    .UniformBuf(1, "GPUClipPlanes", "clipPlanes", GPU_FREQUENCY_PASS)
    .SetTypedefSource("GPU_shader_shared.h")
    .Define("USE_WORLD_CLIP_PLANES");
