GPU_SHADER_CREATE_INFO(gpu_shader_test)
    .vertex_in(0, Type::VEC3, "pos")
    .fragment_out(0, Type::VEC4, "fragColor")
    .push_constant(Type::MAT4, "ModelViewProjectionMatrix")
    .vertex_source("gpu_shader_test_vert.glsl")
    .fragment_source("gpu_shader_test_frag.glsl")
    .do_static_compilation(true);

GPU_SHADER_CREATE_INFO(gpu_compute_1d_test)
    .local_group_size(1)
    .image(1, GPU_RGBA32F, Qualifier::WRITE, ImageType::FLOAT_1D, "canvas")
    .compute_source("gpu_compute_1d_test.glsl")
    .do_static_compilation(true);

GPU_SHADER_CREATE_INFO(gpu_compute_2d_test)
    .local_group_size(1, 1)
    .image(1, GPU_RGBA32F, Qualifier::WRITE, ImageType::FLOAT_2D, "canvas")
    .compute_source("gpu_compute_2d_test.glsl")
    .do_static_compilation(true);
