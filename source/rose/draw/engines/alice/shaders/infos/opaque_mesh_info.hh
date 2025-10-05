#include "gpu_shader_create_info.hh"

/* -------------------------------------------------------------------- */
/** \name Object types
 * \{ */

GPU_SHADER_CREATE_INFO(alice_mesh)
    .vertex_in(0, Type::VEC3, "pos")
    .vertex_in(1, Type::VEC3, "nor")
    .vertex_source("alice_vert.glsl")
	.additional_info("draw_mesh");
	
/** \} */
	
/* -------------------------------------------------------------------- */
/** \name Depth shader types.
 * \{ */

GPU_SHADER_CREATE_INFO(alice_opaque)
	.additional_info("gpu_shader_simple_lighting");

/** \} */

GPU_SHADER_CREATE_INFO(alice_opaque_mesh)
	.additional_info("alice_mesh")
	.additional_info("alice_opaque")
	.do_static_compilation(true);
