#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_context.h"

#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"

#include "UI_interface.h"

#include "ED_numinput.h"
#include "ED_screen.h"

#include "transform.h"
#include "transform_data.h"
#include "transform_constraints.h"
#include "transform_convert.h"
#include "transform_orientations.h"
#include "transform_mode.h"

/* -------------------------------------------------------------------- */
/** \name Transform (Rotation) Matrix Cache
 * \{ */

typedef struct RotateMatrixCache {
	/**
	 * Counter for needed updates (when we need to update to non-default matrix,
	 * we also need another update on next iteration to go back to default matrix,
	 * hence the '2' value used here, instead of a mere boolean).
	 */
	unsigned int do_update_matrix : 2;
	float mat[3][3];
} RotateMatrixCache;

ROSE_INLINE void rmat_cache_init(struct RotateMatrixCache *rmc, const float angle, const float axis[3]) {
	axis_angle_normalized_to_mat3(rmc->mat, axis, angle);
	rmc->do_update_matrix = 0u;
}

ROSE_INLINE void rmat_cache_reset(struct RotateMatrixCache *rmc) {
	rmc->do_update_matrix = 2u;
}

ROSE_INLINE void rmat_cache_update(struct RotateMatrixCache *rmc, const float axis[3], const float angle) {
	if (rmc->do_update_matrix > 0) {
		axis_angle_normalized_to_mat3(rmc->mat, axis, angle);
		rmc->do_update_matrix--;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Transform (Rotation) Element
 * \{ */

void ED_transform_mode_rotation_apply_element_ex(const TransInfo *t, const TransDataContainer *tc, TransData *td, const float mat[3][3], const float *center) {
	float vec[3], totmat[3][3], smat[3][3];
	float eul[3], fmat[3][3], quat[4];

	/* translation */
	sub_v3_v3v3(vec, td->base.center, center);
	mul_m3_v3(mat, vec);
	add_v3_v3(vec, center);
	/* vec now is the location where the object has to be */
	sub_v3_v3(vec, td->base.center);
	mul_m3_v3(td->smtx, vec);

	add_v3_v3v3(td->base.loc, td->base.iloc, vec);

	/* rotation */
	/* euler or quaternion? */
	if (td->extension->rotmode == ROT_MODE_QUAT) {
		/* can be called for texture space translate for example, then opt out */
		if (td->extension->quat) {
			mul_m3_series(fmat, td->smtx, mat, td->mtx);

			if (!is_zero_v3(td->extension->dquat)) {
				/* Correct for delta quat */
				float tmp_mat[3][3];
				quat_to_mat3(tmp_mat, td->extension->dquat);
				mul_m3_m3m3(fmat, fmat, tmp_mat);
			}

			mat3_to_quat(quat, fmat); /* Actual transform */

			if (!is_zero_v4(td->extension->dquat)) {
				/* Correct back for delta quat. */
				float idquat[4];
				invert_qt_qt_normalized(idquat, td->extension->dquat);
				mul_qt_qtqt(quat, idquat, quat);
			}

			mul_qt_qtqt(td->extension->quat, quat, td->extension->iquat);
		}
	}
	else if (td->extension->rotmode == ROT_MODE_AXISANGLE) {
		/* calculate effect based on quats */
		float iquat[4], tquat[4];

		axis_angle_to_quat(iquat, td->extension->iaxis, td->extension->iangle);

		mul_m3_series(fmat, td->smtx, mat, td->mtx);
		mat3_to_quat(quat, fmat); /* Actual transform */
		mul_qt_qtqt(tquat, quat, iquat);

		quat_to_axis_angle(td->extension->axis, td->extension->angle, tquat);
	}
	else {
		/* Calculate the total rotation in eulers. */
		float obmat[3][3];

		mul_m3_m3m3(totmat, mat, td->mtx);
		mul_m3_m3m3(smat, td->smtx, totmat);

		if (!is_zero_v3(td->extension->drot)) {
			/* Correct for delta rot */
			add_eul_euleul(eul, td->extension->irot, td->extension->drot, td->extension->rotmode);
		}
		else {
			copy_v3_v3(eul, td->extension->irot);
		}

		eulO_to_mat3(obmat, eul, td->extension->rotmode);
		mul_m3_m3m3(fmat, smat, obmat);
		mat3_to_compatible_eulO(eul, td->extension->rot, td->extension->rotmode, fmat);

		if (!is_zero_v3(td->extension->drot)) {
			/* Correct back for delta rot. */
			sub_eul_euleul(eul, eul, td->extension->drot, td->extension->rotmode);
		}

		/* and apply */
		copy_v3_v3(td->extension->rot, eul);
	}
}

void ED_transform_mode_rotation_apply_element(const TransInfo *t, const TransDataContainer *tc, TransData *td, const float mat[3][3]) {
	const float *center = td->base.center;

	ED_transform_mode_rotation_apply_element_ex(t, tc, td, mat, center);
}

void ED_transform_mode_rotation_apply_ex(const TransInfo *t, const TransDataContainer *tc, TransData *td, const float axis[3], const float angle, const float step, const bool is_large_rotation, struct RotateMatrixCache *rmc) {
	float t_axis[3];
	const float *final_axis = axis;

	float final_angle = angle;
	if (t->constraint.apply_rot) {
		copy_v3_v3(t_axis, axis);
		t->constraint.apply_rot(t, tc, td, t_axis, NULL);
		final_axis = t_axis;
		
		rmat_cache_reset(rmc);
	}

	/**
	 * Rotation is very likely to be above 180°, we need to do rotation by steps.
	 * Note that this is only needed when doing 'absolute' rotation
	 * (i.e. from initial rotation again, typically when using numinput).
	 * regular incremental rotation (from mouse/widget/...) will be called often enough,
	 * hence steps are small enough to be properly handled without that complicated trick.
	 * Note that we can only do that kind of stepped rotation if we have initial rotation values
	 * (and access to some actual rotation value storage).
	 * Otherwise, just assume it's useless (e.g. in case of mesh/UV/etc. editing).
	 * Also need to be in Euler rotation mode, the others never allow more than one turn anyway.
	 */
	if (is_large_rotation && td->extension != NULL && td->extension->rotmode == ROT_MODE_EUL) {
		copy_v3_v3(td->extension->rot, td->extension->irot);
		for (float angle_progress = step; fabs(angle_progress) < fabs(final_angle); angle_progress += step) {
			axis_angle_normalized_to_mat3(rmc->mat, final_axis, angle_progress);
			ED_transform_mode_rotation_apply_element(t, tc, td, rmc->mat);
		}
		rmat_cache_reset(rmc);
	}
	else if (final_angle != angle) {
		rmat_cache_reset(rmc);
	}

	rmat_cache_update(rmc, final_axis, final_angle);

	ED_transform_mode_rotation_apply_element(t, tc, td, rmc->mat);
}

/** \} */

ROSE_STATIC float large_rotation_limit(float angle) {
	/**
	 * Limit rotation to 1001 turns max
	 * (otherwise iterative handling of 'large' rotations would become too slow).
	 */
	const float angle_max = (float)(M_PI * 2000.0);
	if (fabsf(angle) > angle_max) {
		const float angle_sign = angle < 0.0f ? -1.0f : 1.0f;
		angle = angle_sign * (fmodf(fabsf(angle), (float)(M_PI * 2.0)) + angle_max);
	}
	return angle;
}

void ED_transform_mode_rotation_apply_value(TransInfo *t, float angle, const float axis[3], const bool is_large_rotation) {
	const float sign = angle < 0.0f ? -1.0f : 1.0f;
	/**
	 * We cannot use something too close to 180°, or 'continuous' rotation may fail
	 * due to computing error...
	 */
	const float angle_step = sign * (float)(0.9 * M_PI);

	if (is_large_rotation) {
		angle = large_rotation_limit(angle);
	}

	RotateMatrixCache rmc;
	memset(&rmc, 0, sizeof(RotateMatrixCache));
	rmat_cache_init(&rmc, angle, axis);

	for (TransDataContainer *tc = t->data_container, *tc_end = t->data_container + t->data_container_length; tc != tc_end; tc++) {
		for (TransData *td = tc->data, *td_end = tc->data + tc->data_length; td != td_end; td++) {
			ED_transform_mode_rotation_apply_ex(t, tc, td, axis, angle, angle_step, is_large_rotation, &rmc);
		}
	}
}

void ED_transform_mode_rotation_header(TransInfo *t, char *str, size_t maxlen, float final) {
	size_t ofs = 0;

	if (ED_number_input_has(&t->number)) {
		char c[NUM_STR_REP_LEN];

		ED_number_input_output(&t->number, c);

		ofs += LIB_strnformat(str + ofs, maxlen - ofs, "Rotation: %s %s", &c[0], t->constraint.text);
	}
	else {
		ofs += LIB_strnformat(str + ofs, maxlen - ofs, "Rotation : %.2f%s", RAD2DEGF(final), t->constraint.text);
	}
}

void ED_transform_mode_rotation_apply(TransInfo *t, const int mval[2]) {
	char str[UI_MAX_DRAW_STR];
	float axis_final[3];
	float final = t->values[0] + t->values_modal_offset[0];

	if ((t->constraint.mode & CON_APPLY) && t->constraint.apply_rot) {
		t->constraint.apply_rot(t, NULL, NULL, axis_final, &final);
	}
	else {
		negate_v3_v3(axis_final, t->spacemat[t->axis]);
	}

	if (ED_number_input_apply(&t->number, &final)) {
		/**
		 * We have to limit the amount of turns to a reasonable number here,
		 * to avoid things getting *very* slow, see how ED_transform_mode_rotation_apply_ex() handles those...
		 */
		final = large_rotation_limit(final);
	}

	t->values_final[0] = final;

	ED_transform_mode_rotation_header(t, str, sizeof(str), final);

	const bool is_large_rotation = ED_number_input_has(&t->number);
	ED_transform_mode_rotation_apply_value(t, final, axis_final, is_large_rotation);

	ED_transform_data_recalc(t);

	ED_area_status_text(t->area, str);
}

void ED_transform_mode_rotation_init(TransInfo *t) {
	t->mode = TFM_ROTATION;
	t->transform = ED_transform_mode_rotation_apply;

	ED_mouse_input_mode(t, &t->mouse, INPUT_ANGLE);

	t->index_max = 0;
	t->number.index_max = 0;

	copy_v3_fl(t->number.value, DEG2RAD(1.0));

	transform_orientations_default_modal_set(t, V3D_ORIENT_VIEW);
	transform_orientations_current_set(t);
}