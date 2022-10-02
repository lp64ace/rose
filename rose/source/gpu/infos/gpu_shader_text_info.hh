#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_INTERFACE_INFO(text_iface, "")
    .Flat(Type::VEC4, "color_flat")
    .NoPerspective(Type::VEC2, "texCoord_interp")
    .Flat(Type::INT, "glyph_offset")
    .Flat(Type::IVEC2, "glyph_dim")
    .Flat(Type::INT, "interp_size");

GPU_SHADER_CREATE_INFO(gpu_shader_text)
    .VertexIn(0, Type::VEC4, "pos")
    .VertexIn(1, Type::VEC4, "col")
    .VertexIn(2, Type ::IVEC2, "glyph_size")
    .VertexIn(3, Type ::INT, "offset")
    .VertexOut(text_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .Sampler(0, ImageType::FLOAT_2D, "glyph", GPU_FREQUENCY_PASS)
    .SetVertexSource("gpu_shader_text_vert.glsl")
    .SetFragmentSource("gpu_shader_text_frag.glsl")
    .AdditionalInfo("gpu_srgb_to_framebuffer_space")
    .SetDoStaticCompilation(true);
