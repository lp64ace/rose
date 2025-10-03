#include "gpu_shader_create_info.hh"

/* -------------------------------------------------------------------- */
/** \name Object types
 * \{ */

GPU_SHADER_CREATE_INFO(alice_mesh)
    .vertex_in(0, Type::VEC3, "pos")
    .vertex_source("depth_vert.glsl");
	
/** \} */
	
/* -------------------------------------------------------------------- */
/** \name Depth shader types.
 * \{ */

GPU_SHADER_CREATE_INFO(alice_depth)
	.fragment_out(0, Type::VEC4, "fragColor")
	.fragment_source("depth_frag.glsl");

/** \} */

GPU_SHADER_CREATE_INFO(alice_depth_mesh)
	.additional_info("alice_mesh")
	.additional_info("alice_depth")
	.do_static_compilation(true);
