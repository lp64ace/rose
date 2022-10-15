#include "gpu/intern/gpu_shader_create_info.h"

GPU_SHADER_CREATE_INFO(gpu_shader_default_diffuse)
	.VertexIn(0,Type::VEC3,"pos")
	.VertexIn(1,Type::VEC3,"norm")
	.VertexIn(2,Type::VEC2,"uv")
	.VertexIn(3,Type::IVEC4,"bonei")
	.VertexIn(4,Type::VEC4,"bonew")
	.FragmentOut(0,Type::VEC4,"fragColor")
	.PushConstant(Type::VEC4, "color")
	.PushConstant(Type::MAT4, "ModelViewMatrix")
	.PushConstant(Type::MAT4, "ProjectionMatrix")
	.PushConstant(Type::MAT4, "ModelViewProjectionMatrix")
    .SetVertexSource("gpu_shader_default_diffuse_vert.glsl")
    .SetFragmentSource("gpu_shader_default_diffuse_frag.glsl")
	.SetDoStaticCompilation(true);
