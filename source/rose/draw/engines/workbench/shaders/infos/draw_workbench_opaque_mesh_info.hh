/* -------------------------------------------------------------------- */
/** \name Object types
 * \{ */

GPU_SHADER_CREATE_INFO(workbench_mesh)
    .vertex_in(0, Type::VEC3, "pos")
    .vertex_in(1, Type::VEC3, "nor")
    .vertex_source("workbench_vert.glsl")
	.additional_info("draw_mesh");
	
/** \} */
	
/* -------------------------------------------------------------------- */
/** \name Depth shader types.
 * \{ */

GPU_SHADER_CREATE_INFO(workbench_opaque)
    .vertex_out(smooth_normal_iface)
    .fragment_out(0, Type::VEC4, "fragColor")
	.fragment_out(1, Type::UINT, "objectId")
    .push_constant(Type::BOOL, "forceShadowing")
    .fragment_source("workbench_frag.glsl");

GPU_SHADER_CREATE_INFO(workbench_depth)
    .vertex_out(smooth_normal_iface)
    .fragment_source("gpu_shader_depth_only_frag.glsl");

/** \} */

GPU_SHADER_CREATE_INFO(workbench_depth_mesh)
	.additional_info("workbench_mesh")
	.additional_info("workbench_depth")
	.additional_info("geometry_material")
	.do_static_compilation(true);

GPU_SHADER_CREATE_INFO(workbench_opaque_mesh)
	.additional_info("workbench_mesh")
	.additional_info("workbench_opaque")
	.additional_info("geometry_material")
	.do_static_compilation(true);
