#pragma once

#include "gpu_interface_info.hh"
#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_INTERFACE_INFO(gpu_widget_iface, "")
    .Flat(Type::FLOAT, "discardFac")
    .Flat(Type::FLOAT, "lineWidth")
    .Flat(Type::VEC2, "outRectSize")
    .Flat(Type::VEC4, "borderColor")
    .Flat(Type::VEC4, "embossColor")
    .Flat(Type::VEC4, "outRoundCorners")
    .NoPerspective(Type::FLOAT, "butCo")
    .NoPerspective(Type::VEC2, "uvInterp")
    .NoPerspective(Type::VEC4, "innerColor");

/* TODO(fclem): Share with C code. */
#define MAX_PARAM 12
#define MAX_INSTANCE 6

GPU_SHADER_CREATE_INFO(gpu_shader_2D_widget_shared)
    .Define("MAX_PARAM", STRINGIFY(MAX_PARAM))
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC3, "checkerColorAndSize")
    .VertexOut(gpu_widget_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .SetVertexSource("gpu_shader_2D_widget_base_vert.glsl")
    .SetFragmentSource("gpu_shader_2D_widget_base_frag.glsl")
    .AdditionalInfo("gpu_srgb_to_framebuffer_space");

GPU_SHADER_CREATE_INFO(gpu_shader_2D_widget_base)
    .SetDoStaticCompilation(true)
    /* gl_InstanceID is supposed to be 0 if not drawing instances, but this seems
     * to be violated in some drivers. For example, macOS 10.15.4 and Intel Iris
     * causes T78307 when using gl_InstanceID outside of instance. */
    .Define("widgetID", "0")
    .PushConstant(Type::VEC4, "parameters", MAX_PARAM)
    .AdditionalInfo("gpu_shader_2D_widget_shared");

GPU_SHADER_CREATE_INFO(gpu_shader_2D_widget_base_inst)
    .SetDoStaticCompilation(true)
    .Define("widgetID", "gl_InstanceID")
    .PushConstant(Type::VEC4, "parameters", (MAX_PARAM * MAX_INSTANCE))
    .AdditionalInfo("gpu_shader_2D_widget_shared");

GPU_SHADER_INTERFACE_INFO(gpu_widget_shadow_iface, "").Smooth(Type::FLOAT, "shadowFalloff");

GPU_SHADER_CREATE_INFO(gpu_shader_2D_widget_shadow)
    .SetDoStaticCompilation(true)
    .PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .PushConstant(Type::VEC4, "parameters", 4)
    .PushConstant(Type::FLOAT, "alpha")
    .VertexIn(0, Type::UINT, "vflag")
    .VertexOut(gpu_widget_shadow_iface)
    .FragmentOut(0, Type::VEC4, "fragColor")
    .SetVertexSource("gpu_shader_2D_widget_shadow_vert.glsl")
    .SetFragmentSource("gpu_shader_2D_widget_shadow_frag.glsl");
