#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_2D_image_overlays_stereo_merge)
    .VertexIn(0, Type::VEC2, "pos")
    .FragmentOut(0, Type::VEC4, "overlayColor")
    .FragmentOut(1, Type::VEC4, "imageColor")
    .Sampler(0, ImageType::FLOAT_2D, "imageTexture")
    .Sampler(1, ImageType::FLOAT_2D, "overlayTexture")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::INT, "stereoDisplaySettings")
    .SetVertexSource("gpu_shader_2D_vert.glsl")
    .SetFragmentSource("gpu_shader_image_overlays_stereo_merge_frag.glsl")
    .SetDoStaticCompilation(true);
