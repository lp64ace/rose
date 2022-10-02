#pragma once

#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_INTERFACE_INFO(gpencil_stroke_vert_iface, "geometry_in")
    .Smooth(Type::VEC4, "finalColor")
    .Smooth(Type::FLOAT, "finalThickness");
GPU_SHADER_INTERFACE_INFO(gpencil_stroke_geom_iface, "geometry_out")
    .Smooth(Type::VEC4, "mColor")
    .Smooth(Type::VEC2, "mTexCoord");

GPU_SHADER_CREATE_INFO(gpu_shader_gpencil_stroke)
    .VertexIn(0, Type::VEC4, "color")
    .VertexIn(1, Type::VEC3, "pos")
    .VertexIn(2, Type::FLOAT, "thickness")
    .VertexOut(gpencil_stroke_vert_iface)
    .SetGeometryLayout(GPU_PRIMITIVE_IN_LINES_ADJ, GPU_PRIMITIVE_OUT_TRIANGLE_STRIP, 13)
    .GeometryOut(gpencil_stroke_geom_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .UniformBuf(0, "GPencilStrokeData", "gpencil_stroke_data")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::MAT4, "ProjectionMatrix")
    .SetVertexSource("gpu_shader_gpencil_stroke_vert.glsl")
    .SetGeometrySource("gpu_shader_gpencil_stroke_geom.glsl")
    .SetFragmentSource("gpu_shader_gpencil_stroke_frag.glsl")
    .SetTypedefSource("GPU_shader_shared.h")
    .SetDoStaticCompilation(true);
