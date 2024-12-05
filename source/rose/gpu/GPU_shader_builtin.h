#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

struct GPUShader;

typedef enum BuiltinShader {
	/** Glyph drawing shader used by the "Rose Load Font" module. */
	GPU_SHADER_TEXT,
	/** Draw a texture with a uniform color multiplied. */
	GPU_SHADER_2D_IMAGE_RECT_COLOR,
	/** Merge viewport overlay texture with the render output. */
	GPU_SHADER_2D_IMAGE_OVERLAYS_MERGE,
	GPU_SHADER_2D_IMAGE_OVERLAYS_STEREO_MERGE,
	/** Multi usage widget shaders for drawing buttons and other UI elements. */
	GPU_SHADER_2D_WIDGET_BASE,
	GPU_SHADER_2D_WIDGET_BASE_INST,
	GPU_SHADER_2D_WIDGET_SHADOW,
	/**
	 * Take a single color for all the vertices and a 3D position for each vertex.
	 * \param color: uniform vec4
	 * \param pos: in vec3
	 */
	GPU_SHADER_3D_UNIFORM_COLOR,

	GPU_SHADER_TEST,
} BuiltinShader;

#define GPU_SHADER_BUILTIN_LEN (GPU_SHADER_TEST + 1)

typedef enum ShaderConfig {
	GPU_SHADER_CFG_DEFAULT = 0,
	GPU_SHADER_CFG_CLIPPED = 1,
} ShaderConfig;

#define GPU_SHADER_CFG_LEN (GPU_SHADER_CFG_CLIPPED + 1)

struct GPUShader *GPU_shader_get_builtin_shader_with_config(BuiltinShader shader, ShaderConfig config);

struct GPUShader *GPU_shader_get_builtin_shader(BuiltinShader shader);

void GPU_shader_free_builtin_shaders(void);

#if defined(__cplusplus)
}
#endif
