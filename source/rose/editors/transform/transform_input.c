#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"

#include "LIB_math_base.h"
#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_utildefines.h"

#include "transform.h"

/* -------------------------------------------------------------------- */
/** \name Callbacks for #MouseInput.apply
 * \{ */

ROSE_STATIC void input_vector(TransInfo *t, MouseInput *mi, const double mval[2], float output[3]) {
	transform_view_vector_convert(t, output, mval[0] - mi->imval[0], mval[1] - mi->imval[1]);
}

typedef struct InputAngle_Data {
	double angle;
	double mval_prev[2];
} InputAngle_Data;

ROSE_STATIC void input_angle(TransInfo *t, MouseInput *mi, const double mval[2], float output[3]) {
	struct InputAngle_Data *data = mi->data;
	
	float dir_prev[2];
	float dir_curr[2];

	float center[2] = {UNPACK2(mi->center)};
	sub_v2_v2v2(dir_prev, (const float [2]){UNPACK2(data->mval_prev)}, center);
	sub_v2_v2v2(dir_curr, (const float [2]){UNPACK2(mval)}, center);

	if (normalize_v2(dir_prev) && normalize_v2(dir_curr)) {
		float dphi = angle_normalized_v2v2(dir_prev, dir_curr);

		if (cross_v2v2(dir_prev, dir_curr) > 0.0f) {
			dphi = -dphi;
		}

		data->angle += dphi;

		data->mval_prev[0] = mval[0];
		data->mval_prev[1] = mval[1];
	}

	output[0] = data->angle;
}

ROSE_STATIC void input_spring(TransInfo *t, MouseInput *mi, const double mval[2], float output[3]) {
	double dx, dy;
	float ratio;

	dx = ((double)mi->center[0] - mval[0]);
	dy = ((double)mi->center[1] - mval[1]);
	ratio = hypot(dx, dy) / (double)mi->factor;

	output[0] = ratio;
}

ROSE_STATIC void input_spring_flip(TransInfo *t, MouseInput *mi, const double mval[2], float output[3]) {
	input_spring(t, mi, mval, output);

	/* flip scale */
	/* values can become really big when zoomed in so use longs T26598. */
	if (((int64_t)((int)mi->center[0] - mval[0]) * (int64_t)((int)mi->center[0] - mi->imval[0]) +
		(int64_t)((int)mi->center[1] - mval[1]) * (int64_t)((int)mi->center[1] - mi->imval[1])) < 0) {
		output[0] *= -1.0f;
	}
}

ROSE_STATIC void input_custom_ratio_flip(TransInfo *t, MouseInput *mi, const double mval[2], float output[3]) {
	double length;
	double distance;
	double dx, dy;
	const int *data = mi->data;

	if (data) {
		int mdx, mdy;
		dx = data[2] - data[0];
		dy = data[3] - data[1];

		length = hypot(dx, dy);

		mdx = mval[0] - data[2];
		mdy = mval[1] - data[3];

		distance = (length != 0.0) ? (mdx * dx + mdy * dy) / length : 0.0;

		output[0] = (length != 0.0) ? (double)(distance / length) : 0.0;
	}
}

ROSE_STATIC void input_custom_ratio(TransInfo *t, MouseInput *mi, const double mval[2], float output[3]) {
	input_custom_ratio_flip(t, mi, mval, output);
	output[0] = -output[0];
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Custom 2D Start/End Coordinate API
 *
 * - #INPUT_CUSTOM_RATIO
 * - #INPUT_CUSTOM_RATIO_FLIP
 * \{ */

void ED_mouse_input_set_custom_points(TransInfo *t, MouseInput *mi, const int mval_start[2], const int mval_end[2]) {
	int *data;

	mi->data = MEM_reallocN(mi->data, sizeof(int[4]));

	data = mi->data;

	data[0] = mval_start[0];
	data[1] = mval_start[1];
	data[2] = mval_end[0];
	data[3] = mval_end[1];
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Setup & Handle Mouse Input
 * \{ */

void ED_mouse_input_init(TransInfo *t, MouseInput *mi, const float center[2], const int mval[2]) {
	mi->center[0] = center[0];
	mi->center[1] = center[1];
	mi->imval[0] = mval[0];
	mi->imval[1] = mval[1];
	mi->post = NULL;
	mi->factor = 0.0f;
}

ROSE_INLINE void mouse_input_calculate_factor(MouseInput *mi) {
	mi->factor = len_v2v2_int(mi->center, mi->imval);

	if (mi->factor == 0.0f) {
		mi->factor = 1.0f;
	}
}

void ED_mouse_input_mode(TransInfo *t, MouseInput *mi, eMouseInputMode mode) {
	void *mi_data_prev = mi->data;

	switch(mode) {
		case INPUT_VECTOR: {
			mi->apply = input_vector;
		} break;
		case INPUT_ANGLE: {
			InputAngle_Data *data = MEM_callocN(sizeof(InputAngle_Data), "InputAngle_Data");
			data->mval_prev[0] = mi->imval[0];
			data->mval_prev[1] = mi->imval[1];
			mi->apply = input_angle;
		} break;
		case INPUT_SPRING_FLIP: {
			mouse_input_calculate_factor(mi);
			mi->apply = input_spring_flip;
		} break;
		case INPUT_CUSTOM_RATIO: {
			mi->apply = input_custom_ratio;
		} break;
		default: {
			mi->apply = NULL;
		} break;
	}

	if (mi_data_prev && (mi_data_prev != mi->data)) {
		MEM_freeN(mi_data_prev);
	}
}

void ED_mouse_input_apply(TransInfo *t, MouseInput *mi, const int mval[2], float output[3]) {
	double mval_db[2] = {UNPACK2(mval)};

	if (mi->apply != NULL) {
		mi->apply(t, mi, mval_db, output);
	}

	if (mi->post) {
		mi->post(t, output);
	}
}

/** \} */
