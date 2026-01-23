/* -------------------------------------------------------------------- */
/** \name Object types
 * \{ */

GPU_SHADER_CREATE_INFO(basic_mesh)
    .vertex_in(0, Type::VEC3, "pos")
    .vertex_in(1, Type::VEC3, "nor")
    .vertex_in(2, Type::IVEC4, "defgroup")
    .vertex_in(3, Type::VEC4, "weight")
    .vertex_source("basic_vert.glsl")
	.additional_info("draw_mesh");
	
/** \} */
	
/* -------------------------------------------------------------------- */
/** \name Depth shader types.
 * \{ */

GPU_SHADER_CREATE_INFO(basic_opaque)
	.additional_info("gpu_shader_simple_lighting");

/** \} */

GPU_SHADER_CREATE_INFO(basic_opaque_mesh)
	.additional_info("basic_mesh")
	.additional_info("basic_opaque")
	.do_static_compilation(true);
