/* -------------------------------------------------------------------- */
/** \name Armature Common
 * \{ */

GPU_SHADER_CREATE_INFO(overlay_frag_output)
    .fragment_out(0, Type::VEC4, "fragColor")
    .fragment_out(1, Type::VEC4, "lineOutput");

/** \} */

/* -------------------------------------------------------------------- */
/** \name Armature Shape
 * \{ */

GPU_SHADER_INTERFACE_INFO(overlay_armature_shape_solid_iface, "")
	.smooth(Type::VEC4, "finalColor");

GPU_SHADER_CREATE_INFO(overlay_armature_shape_solid)
    .do_static_compilation(true)
    .vertex_in(0, Type::VEC3, "pos")
    .vertex_in(1, Type::VEC3, "nor")
    /* Per instance. */
    .vertex_in(2, Type::MAT4, "InstanceModelMatrix")
    .depth_write(DepthWrite::GREATER)
    .vertex_out(overlay_armature_shape_solid_iface)
    .vertex_source("armature_shape_solid_vert.glsl")
    .fragment_source("armature_shape_solid_frag.glsl")
    .additional_info("overlay_frag_output", "draw_view");

/** \} */
