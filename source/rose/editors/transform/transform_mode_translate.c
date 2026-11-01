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
#include "transform_orientations.h"
#include "transform_mode.h"

void ED_transform_mode_translation_apply_element(TransInfo *t, TransDataContainer *tc, TransData *td, const float value[3]) {
	float tvec[3];

	if (t->constraint.apply_vec) {
		t->constraint.apply_vec(t, tc, td, value, tvec);
	}
	else {
		copy_v3_v3(tvec, value);
	}

	if (td->base.loc) {
		add_v3_v3v3(td->base.loc, td->base.iloc, tvec);
	}
}

void ED_transform_mode_translation_apply_ex(TransInfo *t, const float value[3]) {
	for (TransDataContainer *tc = t->data_container, *tc_end = t->data_container + t->data_container_length; tc != tc_end; tc++) {
		TransData *td = tc->data;
		for (int i = 0; i < tc->data_length; i++, td++) {
			ED_transform_mode_translation_apply_element(t, tc, td, value);
		}
	}
}

void ED_transform_mode_translation_header(TransInfo *t, const float vec[3], char *str, const size_t maxlen) {
	size_t ofs = 0;
	char dvec_str[NUM_STR_REP_LEN * 3];
	char dist_str[NUM_STR_REP_LEN];

	float dvec[3], dist;
	copy_v3_v3(dvec, vec);

	if (ED_number_input_has(&t->number)) {
		ED_number_input_output(&t->number, dvec_str);

		dist = len_v3(dvec);
	}
	else {
		if (t->constraint.mode & CON_APPLY) {
			int i = 0;
			if (t->constraint.mode & CON_AXIS0) {
				dvec[i++] = dvec[0];
			}
			if (t->constraint.mode & CON_AXIS1) {
				dvec[i++] = dvec[1];
			}
			if (t->constraint.mode & CON_AXIS2) {
				dvec[i++] = dvec[2];
			}
			while (i != 3) {
				dvec[i++] = 0.0f;
			}
		}

		dist = len_v3(dvec);

		char *dvec0 = &dvec_str[0 * NUM_STR_REP_LEN];
		char *dvec1 = &dvec_str[1 * NUM_STR_REP_LEN];
		char *dvec2 = &dvec_str[2 * NUM_STR_REP_LEN];
		LIB_strnformat(dvec0, NUM_STR_REP_LEN, IN_RANGE_INCL(dvec[0], -1e10f, 1e10f) ? "%.4f" : "%.4e", dvec[0]);
		LIB_strnformat(dvec1, NUM_STR_REP_LEN, IN_RANGE_INCL(dvec[1], -1e10f, 1e10f) ? "%.4f" : "%.4e", dvec[1]);
		LIB_strnformat(dvec2, NUM_STR_REP_LEN, IN_RANGE_INCL(dvec[2], -1e10f, 1e10f) ? "%.4f" : "%.4e", dvec[2]);
	}

	LIB_strnformat(dist_str, sizeof(dist_str), IN_RANGE_INCL(dist, -1e10f, 1e10f) ? "%.4f" : "%.4e", dist);

	char *dvec0 = &dvec_str[0 * NUM_STR_REP_LEN];
	char *dvec1 = &dvec_str[1 * NUM_STR_REP_LEN];
	char *dvec2 = &dvec_str[2 * NUM_STR_REP_LEN];

	if (t->constraint.mode & CON_APPLY) {
		ofs += LIB_strnformat(str + ofs, maxlen - ofs, "Translate : %s (%s)%s", dvec0, dist_str, t->constraint.text);
	}
	else {
		ofs += LIB_strnformat(str + ofs, maxlen - ofs, "Translate : vec3(%s, %s, %s) (%s)%s", dvec0, dvec1, dvec2, dist_str, t->constraint.text);
	}
}

void ED_transform_mode_translation_apply(TransInfo *t, const int mval[2]) {
	char str[UI_MAX_DRAW_STR];

	float global_dir[3];
	if ((t->flag & T_INPUT_IS_VALUES_FINAL) != 0) {
		copy_v3_v3(global_dir, t->values);
	}
	else if (ED_number_input_apply(&t->number, global_dir)) {
		if (t->constraint.mode & CON_APPLY) {
			if (t->constraint.mode & CON_AXIS0) {
				mul_v3_v3fl(global_dir, t->spacemat[0], global_dir[0]);
			}
			if (t->constraint.mode & CON_AXIS1) {
				mul_v3_v3fl(global_dir, t->spacemat[1], global_dir[1]);
			}
			if (t->constraint.mode & CON_AXIS2) {
				mul_v3_v3fl(global_dir, t->spacemat[2], global_dir[2]);
			}
		}
		else {
			mul_v3_m3v3(global_dir, t->spacemat, global_dir);
		}
	}
	else {
		copy_v3_v3(global_dir, t->values);
		if (!is_zero_v3(t->values_modal_offset)) {
			float values_ofs[3];
			mul_v3_m3v3(values_ofs, t->spacemat, t->values_modal_offset);
			add_v3_v3(global_dir, values_ofs);
		}

		if (t->constraint.mode & CON_APPLY) {
			float in[3];
			copy_v3_v3(in, global_dir);
			t->constraint.apply_vec(t, NULL, NULL, in, global_dir);
		}
	}

	mul_v3_m3v3(t->values_final, t->spaceinv, global_dir);

	ED_transform_mode_translation_apply_ex(t, global_dir);
	ED_transform_mode_translation_header(t, (t->constraint.mode & CON_APPLY) ? t->values_final : global_dir, str, sizeof(str));

	ED_transform_data_recalc(t);

	ED_area_status_text(t->area, str);
}

void ED_transform_mode_translation_init(struct TransInfo *t) {
	t->transform = ED_transform_mode_translation_apply;

	ED_mouse_input_mode(t, &t->mouse, INPUT_VECTOR);

	t->index_max = 2;
	t->number.flag = 0;
	t->number.index_max = t->index_max;

	transform_orientations_current_set(t);
}
