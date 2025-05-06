GPU_SHADER_CREATE_INFO(gpu_shader_mandelbrot)
	.local_group_size(2, 2)
	.image(1, GPU_RGBA32F, Qualifier::WRITE, ImageType::FLOAT_2D, "canvas")
	.compute_source("gpu_shader_mandelbrot.glsl")
	.do_static_compilation(true);