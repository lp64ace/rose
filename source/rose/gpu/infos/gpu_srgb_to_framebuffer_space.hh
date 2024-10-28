GPU_SHADER_CREATE_INFO(gpu_srgb_to_framebuffer_space)
	.push_constant(Type::BOOL, "srgbTarget")
	.define("rose_srgb_to_framebuffer_space(a) a");
