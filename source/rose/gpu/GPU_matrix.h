#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

struct GPUShader;

void GPU_matrix_reset(void);
void GPU_matrix_push(void);
void GPU_matrix_pop(void);

void GPU_matrix_identity_set(void);

void GPU_matrix_scale_1f(float factor);

/* 3D ModelView Matrix */

void GPU_matrix_set(const float m[4][4]);
void GPU_matrix_mul(const float m[4][4]);

void GPU_matrix_translate_3f(float x, float y, float z);
void GPU_matrix_translate_3fv(const float vec[3]);
void GPU_matrix_scale_3f(float x, float y, float z);
void GPU_matrix_scale_3fv(const float vec[3]);

/**
 * Axis of rotation should be a unit vector.
 */
void GPU_matrix_rotate_3f(float deg, float x, float y, float z);
/**
 * Axis of rotation should be a unit vector.
 */
void GPU_matrix_rotate_3fv(float deg, const float axis[3]);

void GPU_matrix_look_at(float eyeX, float eyeY, float eyeZ, float centerX, float centerY, float centerZ, float upX, float upY, float upZ);

/* 2D ModelView Matrix */

void GPU_matrix_translate_2f(float x, float y);
void GPU_matrix_translate_2fv(const float vec[2]);
void GPU_matrix_scale_2f(float x, float y);
void GPU_matrix_scale_2fv(const float vec[2]);
void GPU_matrix_rotate_2d(float deg);

/* Projection Matrix (2D or 3D). */

void GPU_matrix_push_projection(void);
void GPU_matrix_pop_projection(void);

/* 3D Projection Matrix. */

void GPU_matrix_identity_projection_set(void);
void GPU_matrix_projection_set(const float m[4][4]);

void GPU_matrix_ortho_set(float left, float right, float bottom, float top, float near, float far);
void GPU_matrix_ortho_set_z(float near, float far);

void GPU_matrix_frustum_set(float left, float right, float bottom, float top, float near, float far);
void GPU_matrix_perspective_set(float fovy, float aspect, float near, float far);

/* 2D Projection Matrix. */

void GPU_matrix_ortho_2d_set(float left, float right, float bottom, float top);

/* Functions to get matrix values. */

const float (*GPU_matrix_model_view_get(float m[4][4]))[4];
const float (*GPU_matrix_projection_get(float m[4][4]))[4];
const float (*GPU_matrix_model_view_projection_get(float m[4][4]))[4];

const float (*GPU_matrix_normal_get(float m[3][3]))[3];
const float (*GPU_matrix_normal_inverse_get(float m[3][3]))[3];

/**
 * Set uniform values for currently bound shader.
 */
void GPU_matrix_bind(struct GPUShader *shader);
bool GPU_matrix_dirty_get(void);

/* Not part of the GPU_matrix API, However we need to check these limits in code that calls into these API's. */
#define GPU_MATRIX_ORTHO_CLIP_NEAR_DEFAULT (-100)
#define GPU_MATRIX_ORTHO_CLIP_FAR_DEFAULT (100)

#if defined(__cplusplus)
}
#endif
