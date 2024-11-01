GPU_SHADER_CREATE_INFO(gpu_shader_mandelbrot)
    .local_group_size(4, 4)
    .image(1, GPU_RGBA8, Qualifier::WRITE, ImageType::FLOAT_2D, "canvas")
    .compute_source("gpu_shader_mandelbrot.glsl")
    .do_static_compilation(true);