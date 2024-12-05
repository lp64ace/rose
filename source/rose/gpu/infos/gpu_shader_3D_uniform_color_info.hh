GPU_SHADER_CREATE_INFO(gpu_shader_3D_uniform_color)
	.vertex_in(0, Type::VEC3, "pos")
	.fragment_out(0, Type::VEC4, "fragColor")
	.push_constant(Type::MAT4, "ModelViewProjectionMatrix")
	.push_constant(Type::VEC4, "color")
	.vertex_source("gpu_shader_3D_vert.glsl")
	.fragment_source("gpu_shader_uniform_color_frag.glsl")
	.additional_info("gpu_srgb_to_framebuffer_space")
	.do_static_compilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_uniform_color_clipped)
	.additional_info("gpu_shader_3D_uniform_color")
	.additional_info("gpu_clip_planes")
	.do_static_compilation(true);

GPU_SHADER_CREATE_INFO(gpu_shader_3D_clipped_uniform_color)
	.vertex_in(0, Type::VEC3, "pos")
	.fragment_out(0, Type::VEC4, "fragColor")
	.push_constant(Type::MAT4, "ModelViewProjectionMatrix")
	.push_constant(Type::VEC4, "color")
	.push_constant(Type::MAT4, "ModelMatrix")
	.push_constant(Type::VEC4, "ClipPlane")
	.vertex_source("gpu_shader_3D_clipped_uniform_color_vert.glsl")
	.fragment_source("gpu_shader_uniform_color_frag.glsl")
	.additional_info("gpu_srgb_to_framebuffer_space")
	.do_static_compilation(true);
