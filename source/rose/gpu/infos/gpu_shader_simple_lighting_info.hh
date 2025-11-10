GPU_SHADER_INTERFACE_INFO(smooth_normal_iface, "")
	.smooth(Type::VEC3, "normal");

GPU_SHADER_CREATE_INFO(gpu_shader_simple_lighting)
    .vertex_out(smooth_normal_iface)
    .fragment_out(0, Type::VEC4, "fragColor")
    .fragment_source("gpu_shader_simple_lighting_frag.glsl");
