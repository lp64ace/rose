#include "KER_camera.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_object.h"
#include "KER_scene.h"

#include "LIB_math_base.h"
#include "LIB_math_geom.h"
#include "LIB_math_matrix.h"
#include "LIB_rect.h"

/* -------------------------------------------------------------------- */
/** \name Data-block Creation
 * \{ */

Camera *KER_camera_add(Main *main, const char *name) {
	return KER_id_new(main, ID_CA, name);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Camera Parameter Access
 * \{ */

void KER_camera_params_init(CameraParams *params) {
	memset(params, 0, sizeof(CameraParams));

	/* default, human vertical fov is 120 degrees. */
	params->fov = M_PI_2 * 2.0f / 3.0f;

	/* fallback for non camera objects */
	params->clip_start = 1e-2f;
	params->clip_end = 1e+2f;
}

void KER_camera_params_from_object(CameraParams *params, const Object *obcamera) {
	if (!obcamera) {
		return;
	}

	if (obcamera->type == OB_CAMERA) {
		const Camera *camera = (const Camera *)obcamera->data;

		if (camera->type == CAM_ORTHO) {
			params->use_orthographic |= 1;
		}
	}
}

void KER_camera_params_compute_matrix(CameraParams *params) {
	rctf viewplane = params->viewplane;

	/* compute projection matrix */
	if (params->use_orthographic) {
		orthographic_m4(params->winmat, viewplane.xmin, viewplane.xmax, viewplane.ymin, viewplane.ymax, params->clip_start, params->clip_end);
	}
	else {
		perspective_m4(params->winmat, viewplane.xmin, viewplane.xmax, viewplane.ymin, viewplane.ymax, params->clip_start, params->clip_end);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Camera multi-view API
 * \{ */

ROSE_INLINE void camera_model_matrix(const Object *obcamera, float r_modelmat[4][4]) {
	copy_m4_m4(r_modelmat, KER_object_object_to_world(obcamera));
}

void KER_camera_multiview_view_matrix(const RenderData *rd, const Object *obcamera, float r_viewmat[4][4]) {
	KER_camera_multiview_model_matrix(rd, obcamera, r_viewmat);
	invert_m4(r_viewmat);
}

void KER_camera_multiview_model_matrix(const RenderData *rd, const Object *obcamera, float r_modelmat[4][4]) {
	camera_model_matrix(obcamera, r_modelmat);
}

void KER_camera_multiview_window_matrix(const struct RenderData *rd, const struct Object *obcamera, const float size[2], float r_winmat[4][4]) {
	CameraParams params;

	/* Setup parameters */
	KER_camera_params_init(&params);
	KER_camera_params_from_object(&params, obcamera);

	float tangent = tanf(params.fov * 0.5f);
	float aspect = size[0] / size[1];

	params.viewplane.xmin = -tangent * aspect * params.clip_start;
	params.viewplane.xmax = +tangent * aspect * params.clip_start;
	params.viewplane.ymin = -tangent * params.clip_start;
	params.viewplane.ymax = +tangent * params.clip_start;

	/* Compute matrix, view-plane, etc. */
	KER_camera_params_compute_matrix(&params);

	copy_m4_m4(r_winmat, params.winmat);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Camera Data-Block
 * \{ */

ROSE_INLINE void camera_init_data(ID *id) {
	Camera *camera = (Camera *)id;

	camera->type = CAM_PERSP;
}

ROSE_INLINE void camera_free_data(ID *id) {
	Camera *camera = (Camera *)id;
}

IDTypeInfo IDType_ID_CA = {
	.idcode = ID_CA,

	.filter = FILTER_ID_CA,
	.depends = 0,
	.index = INDEX_ID_CA,
	.size = sizeof(Camera),

	.name = "Camera",
	.name_plural = "Cameras",

	.flag = IDTYPE_FLAGS_NO_COPY | IDTYPE_FLAGS_NO_ANIMDATA,

	.init_data = camera_init_data,
	.copy_data = NULL,
	.free_data = camera_free_data,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
