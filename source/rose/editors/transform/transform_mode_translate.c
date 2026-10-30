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
	char tvec[3][NUM_STR_REP_LEN];
	size_t ofs = 0;
	LIB_strnformat(tvec[0], 16, "%.4f", vec[0]);
	LIB_strnformat(tvec[1], 16, "%.4f", vec[1]);
	LIB_strnformat(tvec[2], 16, "%.4f", vec[2]);

	if (t->constraint.mode & CON_APPLY) {
		ofs += LIB_strnformat(str + ofs, maxlen - ofs, "Translate : %s%s", tvec[0], t->constraint.text);
	}
	else {
		ofs += LIB_strnformat(str + ofs, maxlen - ofs, "Translate : vec3(%s, %s, %s) %s", tvec[0], tvec[1], tvec[2], t->constraint.text);
	}
}

void ED_transform_mode_translation_apply(TransInfo *t, const int mval[2]) {
	char str[UI_MAX_DRAW_STR];

	float global_dir[3];
	if ((t->flag & T_INPUT_IS_VALUES_FINAL) != 0) {
		copy_v3_v3(global_dir, t->values);
	}
	else {
		copy_v3_v3(global_dir, t->values);
		if (!is_zero_v3(t->values_modal_offset)) {
			add_v3_v3(global_dir, t->values_modal_offset);
		}

		if (t->constraint.mode & CON_APPLY) {
			float in[3];
			copy_v3_v3(in, global_dir);
			t->constraint.apply_vec(t, NULL, NULL, in, global_dir);
		}
	}

	ED_transform_mode_translation_apply_ex(t, global_dir);
	ED_transform_mode_translation_header(t, (t->constraint.mode & CON_APPLY) ? t->values_final : global_dir, str, sizeof(str));

	ED_transform_data_recalc(t);

	ED_area_status_text(t->area, str);
}

void ED_transform_mode_translation_init(struct TransInfo *t) {
	t->transform = ED_transform_mode_translation_apply;

	ED_mouse_input_mode(t, &t->mouse, INPUT_VECTOR);
}
