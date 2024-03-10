GPU_SHADER_CREATE_INFO(gpu_shader_test)
    .vertex_in(0, Type::VEC3, "pos")
    .fragment_out(0, Type::VEC4, "fragColor")
    .push_constant(Type::MAT4, "ModelViewProjectionMatrix")
    .vertex_source("gpu_shader_test_vert.glsl")
    .fragment_source("gpu_shader_test_frag.glsl")
    .do_static_compilation(true);