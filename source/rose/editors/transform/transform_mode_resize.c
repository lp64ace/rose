#include "MEM_guardedalloc.h"

#include "KER_context.h"

#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"

#include "UI_interface.h"

#include "ED_screen.h"

#include "transform.h"
#include "transform_data.h"
#include "transform_convert.h"
#include "transform_mode.h"

#define NUM_STR_REP_LEN 16

ROSE_STATIC void transform_mat3_to_size(const float mat[3][3], const float smat[3][3], float size[3]) {
	float rmat[3][3];

	mat3_to_rot_size(rmat, size, mat);

	/* First tried with dot-product... but the sign flip is crucial. */
	if (dot_v3v3(rmat[0], smat[0]) < 0.0f) {
		size[0] = -size[0];
	}
	if (dot_v3v3(rmat[1], smat[1]) < 0.0f) {
		size[1] = -size[1];
	}
	if (dot_v3v3(rmat[2], smat[2]) < 0.0f) {
		size[2] = -size[2];
	}
}

void ED_transform_mode_resize_header(TransInfo *t, const float vec[3], char *str, const int maxlen) {
	char tvec[NUM_STR_REP_LEN * 3];
	size_t ofs = 0;
	LIB_strnformat(&tvec[NUM_STR_REP_LEN * 0], 16, "%.4f", vec[0]);
	LIB_strnformat(&tvec[NUM_STR_REP_LEN * 1], 16, "%.4f", vec[1]);
	LIB_strnformat(&tvec[NUM_STR_REP_LEN * 2], 16, "%.4f", vec[2]);

	if (t->constraint.mode & CON_APPLY) {
		ofs += LIB_strnformat(str + ofs, maxlen - ofs, "Scale : %s%s", &tvec[NUM_STR_REP_LEN * 0], t->constraint.text);
	}
	else {
		ofs += LIB_strnformat(str + ofs, maxlen - ofs, "Scale : vec3(%s, %s, %s) %s", &tvec[NUM_STR_REP_LEN * 0], &tvec[NUM_STR_REP_LEN * 1], &tvec[NUM_STR_REP_LEN * 2], t->constraint.text);
	}
}

void ED_transform_mode_resize_element(const TransInfo *t, const TransDataContainer *tc, TransData *td, const float mat[3][3]) {
	float tmat[3][3];

	copy_m3_m3(tmat, mat);

	if (t->constraint.apply_size) {
		t->constraint.apply_size(t, tc, td, tmat);
	}

	float fsize[3];

	if (ELEM(t->type, TC_OBJECT)) {
		float obsizemat[3][3];
		mul_m3_m3m3(obsizemat, tmat, td->axismtx);
		transform_mat3_to_size(obsizemat, td->axismtx, fsize);
	}
	else {
		mat3_to_size(fsize, tmat);
	}

	td->extension->size[0] = td->extension->isize[0] * (1.0f + (fsize[0] - 1.0f));
	td->extension->size[1] = td->extension->isize[1] * (1.0f + (fsize[1] - 1.0f));
	td->extension->size[2] = td->extension->isize[2] * (1.0f + (fsize[2] - 1.0f));
}

void ED_transform_mode_resize_apply(TransInfo *t, const int mval[2]) {
	char str[UI_MAX_DRAW_STR];

	if ((t->flag & T_INPUT_IS_VALUES_FINAL) != 0) {
		copy_v3_v3(t->values_final, t->values);
	}
	else {
		float ratio = t->values[0];

		copy_v3_fl(t->values_final, ratio);
		add_v3_v3(t->values_final, t->values_modal_offset);
	}

	float mat[3][3];
	size_to_mat3(mat, t->values_final);
	if (t->constraint.mode & CON_APPLY) {
		t->constraint.apply_size(t, NULL, NULL, mat);

		float pvec[3] = {0.0f, 0.0f, 0.0f};

		int j = 0;
		for (int i = 0; i < 3; i++) {
			if (!(t->constraint.mode & (CON_AXIS0 << i))) {
				t->values_final[i] = 1.0f;
			}
			else {
				pvec[j++] = t->values_final[i];
			}
		}

		ED_transform_mode_resize_header(t, pvec, str, sizeof(str));
	}
	else {
		ED_transform_mode_resize_header(t, t->values_final, str, sizeof(str));
	}

	copy_m3_m3(t->mat, mat);

	for (TransDataContainer *tc = t->data_container, *tc_end = t->data_container + t->data_container_length; tc != tc_end; tc++) {
		TransData *td = tc->data;
		for (int i = 0; i < tc->data_length; i++, td++) {
			ED_transform_mode_resize_element(t, tc, td, mat);
		}
	}

	ED_transform_data_recalc(t);

	ED_area_status_text(t->area, str);
}

void ED_transform_mode_resize_init(TransInfo *t, const float mouse_dir_constraint[3]) {
	t->mode = TFM_RESIZE;
	t->transform = ED_transform_mode_resize_apply;

	if (is_zero_v3(mouse_dir_constraint)) {
		ED_mouse_input_mode(t, &t->mouse, INPUT_SPRING_FLIP);
	}
	else {
		int mval_start[3];
		int mval_end[3];
		float mval_dir[3];
		float t_mval[2];
		float viewmat[3][3];

		copy_m3_m4(viewmat, t->viewmat);
		mul_v3_m3v3(mval_dir, viewmat, mouse_dir_constraint);
		normalize_v2(mval_dir);

		if (is_zero_v2(mval_dir)) {
			mval_dir[0] = 0;
			mval_dir[1] = 1;
		}

		mval_start[0] = t->center2d[0];
		mval_start[1] = t->center2d[1];

		t_mval[0] = t->mval[0] - mval_start[0];
		t_mval[1] = t->mval[1] - mval_start[1];
		project_v2_v2v2(mval_dir, t_mval, mval_dir);

		mval_end[0] = t->center2d[0] + mval_dir[0];
		mval_end[1] = t->center2d[1] + mval_dir[1];

		ED_mouse_input_set_custom_points(t, &t->mouse, mval_end, mval_start);
		ED_mouse_input_mode(t, &t->mouse, INPUT_CUSTOM_RATIO);
	}
}
