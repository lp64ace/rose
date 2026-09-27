/* -------------------------------------------------------------------- */
/** \name Outline Interface
 * \{ */

GPU_SHADER_CREATE_INFO(overlay_outline)
	.typedef_source("overlay_shader_shared.h")
	.fragment_source("overlay_effect_outline_frag.glsl")
	.sampler(0, ImageType::UINT_2D, "objectIdBuffer")
	.uniform_buf(4, "WorldData", "worldData", Frequency::PASS)
	.fragment_out(0, Type::VEC4, "fragColor")
	.additional_info("draw_fullscreen")
	.do_static_compilation(true);

/** \} */
