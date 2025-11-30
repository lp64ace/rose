#ifndef KER_CAMERA_H
#define KER_CAMERA_H

#include "DNA_camera_types.h"
#include "DNA_vector_types.h"

struct Camera;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Camera Parameters:
 *
 * Intermediate struct for storing camera parameters from various sources,
 * to unify computation of view-plane, window matrix, ... etc.
 */
typedef struct CameraParams {
	unsigned int use_orthographic : 1;

	 /* clipping */
	float clip_start;
	float clip_end;

	/* computed viewplane */
	rctf viewplane;

	/* computed matrix */
	float winmat[4][4];
} CameraParams;

/* -------------------------------------------------------------------- */
/** \name Data-block Creation
 * \{ */

struct Camera *KER_camera_add(struct Main *main, const char *name);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Camera multi-view API
 * \{ */

void KER_camera_multiview_view_matrix(const struct RenderData *rd, const struct Object *obcamera, float r_viewmat[4][4]);
void KER_camera_multiview_model_matrix(const struct RenderData *rd, const struct Object *obcamera, float r_modelmat[4][4]);

void KER_camera_multiview_window_matrix(const struct RenderData *rd, const struct Object *obcamera, float r_modelmat[4][4]);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // KER_CAMERA_H
