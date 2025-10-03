#pragma once

#include "GPU_shader_builtin.h"

#include <stdbool.h>

/**
 * Hardware limit is 16. Position attribute is always needed so we reduce to 15.
 * This makes sure the GPUVertexFormat name buffer does not overflow.
 */
#define GPU_MAX_ATTR 15

/* Determined by the maximum uniform buffer size divided by chunk size. */
#define GPU_MAX_UNIFORM_ATTR 8

#if defined(__cplusplus)
extern "C" {
#endif

struct GPUVertBuf;

/** Opaque type hiding #rose::gpu::shader::ShaderCreateInfo */
typedef struct GPUShaderCreateInfo GPUShaderCreateInfo;
/** Opaque type hiding #rose::gpu::Shader */
typedef struct GPUShader GPUShader;

/* -------------------------------------------------------------------- */
/** \name Creation
 * \{ */

/** Create a shader using the given #GPUShaderCreateInfo, can return NULL pointer if compilation fails. */
GPUShader *GPU_shader_create_from_info(const GPUShaderCreateInfo *info);

/** Create a shader using a named #GPUShaderCreateInfo registered on startup. */
GPUShader *GPU_shader_create_from_info_name(const char *info_name);

/** Fetch a named #GPUShaderCreateInfo registered at startup. */
const GPUShaderCreateInfo *GPU_shader_create_info_get(const char *info_name);

bool GPUshader_create_info_check_error(const GPUShaderCreateInfo *info, char r_error[128]);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Free
 * \{ */

void GPU_shader_free(GPUShader *shader);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Binding
 * \{ */

/**
 * Set the given shader as active shader for the active GPU context.
 * It replaces any already bound shader.
 * All following draw-calls and dispatches will use this shader.
 * Uniform functions need to have the shader bound in order to work. (TODO: until we use
 * glProgramUniform)
 */
void GPU_shader_bind(GPUShader *shader);

/**
 * Unbind the active shader.
 * \note this is a no-op in release builds. But it make sense to actually do it in user land code
 * to detect incorrect API usage.
 */
void GPU_shader_unbind(void);

/**
 * Return the currently bound shader to the active GPU context.
 * \return NULL pointer if no shader is bound of if no context is active.
 */
GPUShader *GPU_shader_get_bound(void);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Uniform API.
 * \{ */

/**
 * Returns binding point location.
 * Binding location are given to be set at shader compile time and immutable.
 */
int GPU_shader_get_ubo_binding(GPUShader *shader, const char *name);
int GPU_shader_get_ssbo_binding(GPUShader *shader, const char *name);
int GPU_shader_get_sampler_binding(GPUShader *shader, const char *name);

/**
 * Returns uniform location.
 * If cached, it is faster than querying the interface for each uniform assignment.
 */
int GPU_shader_get_uniform(GPUShader *shader, const char *name);

/**
 * Sets a generic push constant (a.k.a. uniform).
 * \a length and \a array_size should match the create info push_constant declaration.
 */
void GPU_shader_uniform_float_ex(GPUShader *shader, int location, int length, int array_size, const float *value);
void GPU_shader_uniform_int_ex(GPUShader *shader, int location, int length, int array_size, const int *value);

/**
 * Sets a generic push constant (a.k.a. uniform).
 * \a length and \a array_size should match the create info push_constant declaration.
 * These functions need to have the shader bound in order to work. (TODO: until we use
 * glProgramUniform)
 */
void GPU_shader_uniform_1i(GPUShader *shader, const char *name, int value);
void GPU_shader_uniform_1b(GPUShader *shader, const char *name, bool value);
void GPU_shader_uniform_1f(GPUShader *shader, const char *name, float value);
void GPU_shader_uniform_2f(GPUShader *shader, const char *name, float x, float y);
void GPU_shader_uniform_3f(GPUShader *shader, const char *name, float x, float y, float z);
void GPU_shader_uniform_4f(GPUShader *shader, const char *name, float x, float y, float z, float w);
void GPU_shader_uniform_2fv(GPUShader *shader, const char *name, const float data[2]);
void GPU_shader_uniform_3fv(GPUShader *shader, const char *name, const float data[3]);
void GPU_shader_uniform_4fv(GPUShader *shader, const char *name, const float data[4]);
void GPU_shader_uniform_2iv(GPUShader *shader, const char *name, const int data[2]);
void GPU_shader_uniform_mat4(GPUShader *shader, const char *name, const float data[4][4]);
void GPU_shader_uniform_mat3_as_mat4(GPUShader *shader, const char *name, const float data[3][3]);
void GPU_shader_uniform_1f_array(GPUShader *shader, const char *name, int len, const float *val);
void GPU_shader_uniform_2fv_array(GPUShader *shader, const char *name, int len, const float (*val)[2]);
void GPU_shader_uniform_4fv_array(GPUShader *shader, const char *name, int len, const float (*val)[4]);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Attribute API.
 *
 * Used to create #GPUVertexFormat from the shader's vertex input layout.
 * \{ */

/** Fetch the number of attributes that are active in this shader. */
unsigned int GPU_shader_get_attribute_len(const GPUShader *shader);

/** Fetch the index of an attribute from its name. */
int GPU_shader_get_attribute(const GPUShader *shader, const char *name);

/** Fetch information about the name and type of a specified attribute. */
bool GPU_shader_get_attribute_info(const GPUShader *shader, int attr_location, char r_name[256], int *r_type);

/* \} */

/* -------------------------------------------------------------------- */
/** \name Legacy API
 *
 * All of this section is deprecated and should be ported to use the API described above.
 * \{ */

/**
 * Indexed commonly used uniform name for faster lookup into the uniform cache.
 */
typedef enum UniformBuiltin {
	GPU_UNIFORM_MODEL = 0,		/* mat4 ModelMatrix */
	GPU_UNIFORM_VIEW,			/* mat4 ViewMatrix */
	GPU_UNIFORM_MODELVIEW,		/* mat4 ModelViewMatrix */
	GPU_UNIFORM_PROJECTION,		/* mat4 ProjectionMatrix */
	GPU_UNIFORM_VIEWPROJECTION, /* mat4 ViewProjectionMatrix */
	GPU_UNIFORM_MVP,			/* mat4 ModelViewProjectionMatrix */

	GPU_UNIFORM_MODEL_INV,			/* mat4 ModelMatrixInverse */
	GPU_UNIFORM_VIEW_INV,			/* mat4 ViewMatrixInverse */
	GPU_UNIFORM_MODELVIEW_INV,		/* mat4 ModelViewMatrixInverse */
	GPU_UNIFORM_PROJECTION_INV,		/* mat4 ProjectionMatrixInverse */
	GPU_UNIFORM_VIEWPROJECTION_INV, /* mat4 ViewProjectionMatrixInverse */

	GPU_UNIFORM_NORMAL,		/* mat3 NormalMatrix */
	GPU_UNIFORM_ORCO,		/* vec4 OrcoTexCoFactors[] */
	GPU_UNIFORM_CLIPPLANES, /* vec4 WorldClipPlanes[] */

	GPU_UNIFORM_COLOR,			/* vec4 color */
	GPU_UNIFORM_BASE_INSTANCE,	/* int baseInstance */
	GPU_UNIFORM_RESOURCE_CHUNK, /* int resourceChunk */
	GPU_UNIFORM_RESOURCE_ID,	/* int resourceId */
	GPU_UNIFORM_SRGB_TRANSFORM, /* bool srgbTarget */
} UniformBuiltin;

#define GPU_NUM_UNIFORMS (GPU_UNIFORM_SRGB_TRANSFORM + 1)

/**
 * TODO: To be moved as private API. Not really used outside of gpu_matrix.cc and doesn't really
 * offer a noticeable performance boost.
 */
int GPU_shader_get_builtin_uniform(GPUShader *shader, int builtin);

/* \} */

#if defined(__cplusplus)
}
#endif
