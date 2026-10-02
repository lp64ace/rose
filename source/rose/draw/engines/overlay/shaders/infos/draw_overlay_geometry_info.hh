/* -------------------------------------------------------------------- */
/** \name Overlay Geometry Common
 * \{ */

GPU_SHADER_CREATE_INFO(overlay_geometry_prepass)
    .fragment_out(0, Type::VEC4, "fragColor")
	.fragment_out(1, Type::UINT, "objectId")
    .fragment_source("overlay_geometry_prepass_frag.glsl");

/** \} */

/* -------------------------------------------------------------------- */
/** \name Overlay Geometry Mesh
 * \{ */

GPU_SHADER_CREATE_INFO(overlay_geometry_mesh)
    .vertex_in(0, Type::VEC3, "pos")
    .vertex_source("overlay_vert.glsl")
	.additional_info("draw_mesh");

GPU_SHADER_CREATE_INFO(overlay_geometry_prepass_mesh)
	.additional_info("geometry_material")
    .additional_info("overlay_geometry_mesh")
	.additional_info("overlay_geometry_prepass")
	.do_static_compilation(true);

/** \} */