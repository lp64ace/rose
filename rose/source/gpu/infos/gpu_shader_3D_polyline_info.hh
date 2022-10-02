#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_INTERFACE_INFO(gpu_shader_3D_polyline_iface, "interp")
    .Smooth(Type::VEC4, "color")
    .Smooth(Type::FLOAT, "clip")
    .NoPerspective(Type::FLOAT, "smoothline");

GPU_SHADER_CREATE_INFO(gpu_shader_3D_polyline)
    .Define("SMOOTH_WIDTH", "1.0")
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC2, "viewportSize")
    .PushConstant(Type::FLOAT, "lineWidth")
    .PushConstant(Type::BOOL, "lineSmooth")
    .VertexIn(0, Type::VEC3, "pos")
    .VertexOut(gpu_shader_3D_polyline_iface)
    .SetGeometryLayout(GPU_PRIMITIVE_OUT_LINES, GPU_PRIMITIVE_OUT_TRIANGLE_STRIP, 4)
    .GeometryOut(gpu_shader_3D_polyline_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .SetVertexSource("gpu_shader_3D_polyline_vert.glsl")
    .SetGeometrySource("gpu_shader_3D_polyline_geom.glsl")
    .SetFragmentSource("gpu_shader_3D_polyline_frag.glsl")
    .AdditionalInfo("gpu_srgb_to_framebuffer_space");

GPU_SHADER_CREATE_INFO(gpu_shader_3D_polyline_uniform_color)
    .SetDoStaticCompilation(true)
    .Define("UNIFORM")
    .PushConstant(Type::VEC4, "color")
    .AdditionalInfo("gpu_shader_3D_polyline");

GPU_SHADER_CREATE_INFO(gpu_shader_3D_polyline_uniform_color_clipped)
    .SetDoStaticCompilation(true)
    /* TODO(fclem): Put in a UBO to fit the 128byte requirement. */
    .PushConstant(Type::MAT4, "ModelMatrix")
    .PushConstant(Type::VEC4, "ClipPlane")
    .Define("CLIP")
    .AdditionalInfo("gpu_shader_3D_polyline_uniform_color");

GPU_SHADER_CREATE_INFO(gpu_shader_3D_polyline_flat_color)
    .SetDoStaticCompilation(true)
    .Define("FLAT")
    .VertexIn(1, Type::VEC4, "color")
    .AdditionalInfo("gpu_shader_3D_polyline");

GPU_SHADER_CREATE_INFO(gpu_shader_3D_polyline_smooth_color)
    .SetDoStaticCompilation(true)
    .Define("SMOOTH")
    .VertexIn(1, Type::VEC4, "color")
    .AdditionalInfo("gpu_shader_3D_polyline");
