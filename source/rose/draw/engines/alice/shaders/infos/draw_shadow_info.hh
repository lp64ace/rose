#include "intern/draw_defines.h"

/* -------------------------------------------------------------------- */
/** \name Common
 * \{ */

GPU_SHADER_INTERFACE_INFO(alice_shadow_iface, "vData")
	.smooth(Type::VEC3, "pos")
	.smooth(Type::VEC4, "frontPosition")
	.smooth(Type::VEC4, "backPosition");

GPU_SHADER_CREATE_INFO(alice_shadow_common)
    .vertex_in(0, Type::VEC3, "pos")
    .vertex_in(2, Type::IVEC4, "defgroup")
    .vertex_in(3, Type::VEC4, "weight")
    .vertex_out(alice_shadow_iface)
    .push_constant(Type::FLOAT, "lightDistance")
    .push_constant(Type::VEC3, "lightDirection")
    .vertex_source("alice_shadow_vert.glsl")
    .additional_info("draw_mesh");

/** \} */

/* -------------------------------------------------------------------- */
/** \name Manifold Type
 * \{ */

GPU_SHADER_CREATE_INFO(alice_shadow_manifold)
    .geometry_layout(PrimitiveIn::LINES_ADJACENCY, PrimitiveOut::TRIANGLE_STRIP, 4, 1)
    .geometry_source("alice_shadow_geom.glsl");

GPU_SHADER_CREATE_INFO(alice_shadow_no_manifold)
    .geometry_layout(PrimitiveIn::LINES_ADJACENCY, PrimitiveOut::TRIANGLE_STRIP, 4, 2)
    .geometry_source("alice_shadow_geom.glsl");

/** \} */

/* -------------------------------------------------------------------- */
/** \name Caps Type
 * \{ */

GPU_SHADER_CREATE_INFO(alice_shadow_caps)
    .geometry_layout(PrimitiveIn::TRIANGLES, PrimitiveOut::TRIANGLE_STRIP, 3, 2)
    .geometry_source("alice_shadow_caps_geom.glsl");

/** \} */

/* -------------------------------------------------------------------- */
/** \name Debug Type
 * \{ */

GPU_SHADER_CREATE_INFO(alice_shadow_no_debug)
    .fragment_source("gpu_shader_depth_only_frag.glsl");

/** \} */

/* -------------------------------------------------------------------- */
/** \name Variations Declaration
 * \{ */

#define ALICE_SHADOW_VARIATIONS(suffix, ...) \
  GPU_SHADER_CREATE_INFO(alice_shadow_pass_manifold_no_caps##suffix) \
      .define("SHADOW_PASS") \
      .additional_info("alice_shadow_common", "alice_shadow_manifold", __VA_ARGS__) \
      .do_static_compilation(true); \
  GPU_SHADER_CREATE_INFO(alice_shadow_pass_no_manifold_no_caps##suffix) \
      .define("SHADOW_PASS") \
      .define("DOUBLE_MANIFOLD") \
      .additional_info("alice_shadow_common", "alice_shadow_no_manifold", __VA_ARGS__) \
      .do_static_compilation(true); \
  GPU_SHADER_CREATE_INFO(alice_shadow_fail_manifold_caps##suffix) \
      .define("SHADOW_FAIL") \
      .additional_info("alice_shadow_common", "alice_shadow_caps", __VA_ARGS__) \
      .do_static_compilation(true); \
  GPU_SHADER_CREATE_INFO(alice_shadow_fail_manifold_no_caps##suffix) \
      .define("SHADOW_FAIL") \
      .additional_info("alice_shadow_common", "alice_shadow_manifold", __VA_ARGS__) \
      .do_static_compilation(true); \
  GPU_SHADER_CREATE_INFO(alice_shadow_fail_no_manifold_caps##suffix) \
      .define("SHADOW_FAIL") \
      .define("DOUBLE_MANIFOLD") \
      .additional_info("alice_shadow_common", "alice_shadow_caps", __VA_ARGS__) \
      .do_static_compilation(true); \
  GPU_SHADER_CREATE_INFO(alice_shadow_fail_no_manifold_no_caps##suffix) \
      .define("SHADOW_FAIL") \
      .define("DOUBLE_MANIFOLD") \
      .additional_info("alice_shadow_common", "alice_shadow_no_manifold", __VA_ARGS__) \
      .do_static_compilation(true);


ALICE_SHADOW_VARIATIONS(, "alice_shadow_no_debug")

/** \} */
