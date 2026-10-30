#include "LIB_math_matrix.h"
#include "LIB_math_geom.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"
#include "LIB_utildefines.h"

#include "transform.h"
#include "transform_data.h"
#include "transform_constraints.h"

/* -------------------------------------------------------------------- */
/** \name Constraints
 * \{ */

ROSE_STATIC void constraint_axis_correct_center(const TransInfo *t, float t_con_center[3]) {
	if (t->spacetype == SPACE_VIEW3D) {
		// View3D *v3d = t->area->spacedata.first;
		const float min_dist = 1.0f; /* v3d->clip_start; */
		float dir[3];
		float l;

		sub_v3_v3v3(dir, t_con_center, t->viewinv[3]);
		if (dot_v3v3(dir, t->viewinv[2]) < 0.0f) {
			negate_v3(dir);
		}
		project_v3_v3v3(dir, dir, t->viewinv[2]);

		l = len_v3(dir);

		if (l < min_dist) {
			float diff[3];
			normalize_v3_v3_length(diff, t->viewinv[2], min_dist - l);
			sub_v3_v3(t_con_center, diff);
		}
	}
}

ROSE_STATIC void view_vector_calc(const TransInfo *t, const float focus[3], float r_vec[3]) {
	sub_v3_v3v3(r_vec, t->viewinv[3], focus);
	normalize_v3(r_vec);
}

ROSE_STATIC void constraint_axis_projection(const TransInfo *t, const float axis[3], const float in[3], float out[3]) {
	float norm[3], vec[3];
	float t_con_center[3];

	if (is_zero_v3(in)) {
		return;
	}

	copy_v3_v3(t_con_center, t->center_global);

	constraint_axis_correct_center(t, t_con_center);

	float angle = fabsf(angle_v3v3(axis, t->viewinv[2]));
	if (angle > M_PI_2) {
		angle = M_PI - angle;
	}

	if (angle < DEG2RAD(0.5f)) {
		project_v3_v3v3(vec, in, t->viewinv[1]);

		float factor = dot_v3v3(t->viewinv[1], vec) * 2.0f;
		if (factor < 0.0f) {
			factor *= -factor;
		}
		else {
			factor *= factor;
		}

		normalize_v3_v3_length(out, axis, -factor);
	}
	else {
		float v[3];
		float norm_center[3];
		float plane[3];

		view_vector_calc(t, t_con_center, norm_center);
		cross_v3_v3v3(plane, norm_center, axis);

		project_v3_v3v3(vec, in, plane);
		sub_v3_v3v3(vec, in, vec);

		add_v3_v3v3(v, vec, t_con_center);
		view_vector_calc(t, v, norm);

		/* give arbitrary large value if projection is impossible */
		float factor = dot_v3v3(axis, norm);
		if (1.0f - fabsf(factor) < 2e-4f) {
			copy_v3_v3(out, axis);
			if (factor < 0) {
				mul_v3_fl(out, -1e9f);
			}
			else {
				mul_v3_fl(out, 1e9f);
			}
		}
		else {
			float mul;
			if (isect_ray_ray_v3(t_con_center, axis, v, norm, &mul, NULL)) {
				mul_v3_v3fl(out, axis, mul);
			}
			else {
				ROSE_assert_unreachable(); // How?!
			}

			if (!isfinite(out[0])) {
				out[0] = 0.0f;
			}
			if (!isfinite(out[1])) {
				out[1] = 0.0f;
			}
			if (!isfinite(out[2])) {
				out[2] = 0.0f;
			}
		}
	}
}

/**
 * Generic callback for constant spatial constraints applied to resize motion.
 */
ROSE_STATIC void constraint_apply_axis_size(const TransInfo *t, const TransDataContainer *UNUSED(tc), const TransData *td, float r_smat[3][3]) {
	if (!td && (t->constraint.mode & CON_APPLY) != 0) {
		if (!(t->constraint.mode & CON_AXIS0)) {
			r_smat[0][0] = 1.0f;
		}
		if (!(t->constraint.mode & CON_AXIS1)) {
			r_smat[1][1] = 1.0f;
		}
		if (!(t->constraint.mode & CON_AXIS2)) {
			r_smat[2][2] = 1.0f;
		}
	}
}

ROSE_STATIC void constraint_rotation_impl(const TransInfo *t, float r_axis[3], float *r_angle) {
	int mode = t->constraint.mode & (CON_AXIS0 | CON_AXIS1 | CON_AXIS2);

	switch (mode) {
		case CON_AXIS0:
		case (CON_AXIS1 | CON_AXIS2):
			copy_v3_fl3(r_axis, 1.0f, 0.0f, 0.0f);
			break;
		case CON_AXIS1:
		case (CON_AXIS0 | CON_AXIS2):
			copy_v3_fl3(r_axis, 0.0f, 1.0f, 0.0f);
			break;
		case CON_AXIS2:
		case (CON_AXIS0 | CON_AXIS1):
			copy_v3_fl3(r_axis, 0.0f, 0.0f, 1.0f);
			break;
	}

	if (r_angle) {
		float view_vector[3];
		view_vector_calc(t, t->center_global, view_vector);
		if (dot_v3v3(r_axis, view_vector) > 0.0f) {
			*r_angle = -(*r_angle);
		}
	}
}

ROSE_STATIC void constraint_apply_axis_rot(const TransInfo *t, const TransDataContainer *tc, const TransData *td, float r_axis[3], float *r_angle) {
	if (!td && (t->constraint.mode & CON_APPLY) != 0) {
		constraint_rotation_impl(t, r_axis, r_angle);
	}
}

ROSE_STATIC void constraint_apply_axis_vec(const TransInfo *t, const TransDataContainer *tc, const TransData *td, const float in[3], float out[3]) {
	copy_v3_v3(out, in);

	if (!td && (t->constraint.mode & CON_APPLY) != 0) {
		float c[3];

		if ((t->constraint.mode & CON_AXIS0) != 0) {
			copy_v3_v3(c, t->spacemat[0]);
		}
		else if ((t->constraint.mode & CON_AXIS1) != 0) {
			copy_v3_v3(c, t->spacemat[1]);
		}
		else if ((t->constraint.mode & CON_AXIS2) != 0) {
			copy_v3_v3(c, t->spacemat[2]);
		}

		constraint_axis_projection(t, c, in, out);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Internal Setup Calls
 * \{ */

void transform_constraint_set(TransInfo *t, int constraint, const char text[]) {
	LIB_strnformat(t->constraint.text + 1, sizeof(t->constraint.text), text, "global");
	t->constraint.mode = constraint;
	
	transform_constraint_start(t);

	t->constraint.apply_vec = constraint_apply_axis_vec;
	t->constraint.apply_size = constraint_apply_axis_size;
	t->constraint.apply_rot = constraint_apply_axis_rot;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Start / Stop Constraints
 * \{ */

void transform_constraint_start(TransInfo *t) {
	t->constraint.mode |= CON_APPLY;
	*t->constraint.text = ' ';
}

void transform_constraint_stop(TransInfo *t) {
	t->constraint.mode &= ~CON_APPLY;
	*t->constraint.text = '\0';
}

/** \} */
