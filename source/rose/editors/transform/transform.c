#include "MEM_guardedalloc.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_layer.h"

#include "LIB_math_base.h"
#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "ED_view3d.h"

#include "WM_api.h"
#include "WM_event.h"

#include "transform.h"
#include "transform_data.h"
#include "transform_convert.h"
#include "transform_constraints.h"
#include "transform_mode.h"

/* -------------------------------------------------------------------- */
/** \name Transform API
 * \{ */

ROSE_INLINE eTransformMode transform_mode_really_used(rContext *C, eTransformMode mode) {
	return mode;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Space Dependent Code
 * \{ */

ROSE_INLINE void project_int_view(TransInfo *t, const float vec[3], int adr[2]) {
	if (t->spacetype == SPACE_VIEW3D) {
		if (t->region->regiontype == RGN_TYPE_WINDOW) {
			if (ED_view3d_project_int_global(t->region, vec, adr, V3D_PROJ_TEST_NOP) != V3D_PROJ_RET_OK) {
				adr[0] = (int)INT_MAX;
				adr[1] = (int)INT_MAX;
			}
		}
	}
	else {
		/** What are we transforming?! */
		ROSE_assert_unreachable();
	}
}

ROSE_INLINE void project_float_view(TransInfo *t, const float vec[3], float adr[2]) {
	switch (t->spacetype) {
		case SPACE_VIEW3D: {
			if (t->region->regiontype == RGN_TYPE_WINDOW) {
				if (ED_view3d_project_float_global(t->region, vec, adr, V3D_PROJ_TEST_NOP) != V3D_PROJ_RET_OK) {
					/** What?! */
					adr[0] = t->region->sizex / 2;
					adr[1] = t->region->sizey / 2;
				}
			}
		} break;
		default: {
			int a[2] = {0, 0};
			project_int_view(t, vec, a);
			adr[0] = a[0];
			adr[1] = a[1];
		} break;
	}
}

ROSE_STATIC void transform_view_matrices_center_2d_init(TransInfo *t) {
	project_float_view(t, t->center_global, t->center2d);
}

ROSE_STATIC void transform_view_matrices_center_local_init(TransInfo *t, const float center_global[3]) {
	for (TransDataContainer *tc = t->data_container, *tc_end = t->data_container + t->data_container_length; tc != tc_end; tc++) {
		if (tc->use_local_mat) {
			mul_v3_m4v3(tc->center_local, tc->invmat, center_global);
		}
		else {
			copy_v3_v3(tc->center_local, center_global);
		}
	}
}

ROSE_STATIC void transform_view_matrices_init(TransInfo *t) {
	unit_m4(t->viewmat);
	unit_m4(t->viewinv);
	unit_m4(t->persmat);
	unit_m4(t->persinv);
	
	transform_view_matrices_center_2d_init(t);
	transform_view_matrices_center_local_init(t, t->center_global);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Transform API (Generics)
 * \{ */

ROSE_INLINE void *t_view_get(TransInfo *t) {
	if (t->spacetype == SPACE_VIEW3D) {
		View3D *v3d = t->area->spacedata.first;
		return (void *)v3d;
	}
	if (t->region) {
		return (void *)&t->region->v2d;
	}
	return NULL;
}

void ED_transform_init_info(rContext *C, TransInfo *t, wmOperator *op, const wmEvent *event) {
	t->depsgraph = CTX_data_depsgraph_pointer(C);
	t->scene = CTX_data_scene(C);
	t->view_layer = CTX_data_view_layer(C);
	t->area = CTX_wm_area(C);
	t->region = CTX_wm_region(C);

	t->flag = 0;

	if (t->data_container == NULL) {
		t->data_container = MEM_callocN(sizeof(*t->data_container), __func__);
		t->data_container_length = 1;
	}
	
	int mval[2];
	if (event) {
		if ((t->flag & T_EVENT_DRAG_START)) {
			WM_event_drag_start_mval(event, t->region, mval);
		}
		else {
			copy_v2_v2_int(mval, event->mouse_local);
			t->flag |= T_MODAL;
		}
	}
	else {
		zero_v2_int(mval);
	}

	copy_v2_v2_int(t->mval, mval);
	copy_v2_v2_int(t->mouse.imval, mval);

	t->data_length_all = 0;

	unit_m3(t->mat);
	zero_v3(t->center_global);

	/** By default we operate on the Z-axis */
	t->axis = 2;

	if (t->area == NULL) {
		t->spacetype = SPACE_EMPTY;
	}
	else if ((t->region == NULL) && (t->area->spacetype == SPACE_VIEW3D)) {
		t->spacetype = SPACE_EMPTY;
	}
	else {
		t->spacetype = t->area->spacetype;
	}

	t->view = t_view_get(t);

	bool t_values_set_is_array = false;

	PropertyRNA *property;
	if (op && (property = RNA_struct_find_property(op->ptr, "value")) && RNA_property_is_set(op->ptr, property)) {
		float values[4] = {0};
		if (RNA_property_array_check(property)) {
			RNA_property_float_get_array(op->ptr, property, values);
			t_values_set_is_array |= true;
		}
		else {
			values[0] = RNA_property_float_get(op->ptr, property);
		}

		if ((t->flag & T_MODAL) != 0) {
			copy_v4_v4(t->values_modal_offset, values);
		}
		else {
			copy_v4_v4(t->values, values);
			t->flag |= T_INPUT_IS_VALUES_FINAL;
		}
	}

	if (op && (property = RNA_struct_find_property(op->ptr, "constraint_axis")) && RNA_property_is_set(op->ptr, property)) {
		bool constraint_axis[3] = {false, false, false};
		if (t_values_set_is_array && (t->flag & T_INPUT_IS_VALUES_FINAL) != 0) {
			/**
			 * For operators whose `t->values` is array (as Move and Scale), set constraint so that the
			 * orientation is more intuitive in the Redo Panel.
			 * */
			constraint_axis[0] = constraint_axis[1] = constraint_axis[2] = true;
		}
		else if (RNA_property_is_set(op->ptr, property)) {
			RNA_property_boolean_get_array(op->ptr, property, constraint_axis);
		}

		if (constraint_axis[0] || constraint_axis[1] || constraint_axis[2]) {
			t->constraint.mode |= CON_APPLY;

			SET_FLAG_FROM_TEST(t->constraint.mode, constraint_axis[0], CON_AXIS0);
			SET_FLAG_FROM_TEST(t->constraint.mode, constraint_axis[1], CON_AXIS1);
			SET_FLAG_FROM_TEST(t->constraint.mode, constraint_axis[2], CON_AXIS2);
		}
	}

	transform_view_matrices_init(t);
}

ROSE_STATIC void transform_center_calculate(TransInfo *t) {
	transform_view_matrices_center_local_init(t, t->center_global);
	transform_view_matrices_center_2d_init(t);

	if ((t->options & CTX_OBJECT) != 0) {
		if (t->spacetype == SPACE_VIEW3D && t->region && t->region->regiontype == RGN_TYPE_WINDOW) {
			/** No-op */
		}
	}

	if (t->spacetype == SPACE_VIEW3D) {
		if (t->region->regiontype == RGN_TYPE_WINDOW) {
			t->zfac = ED_view3d_calc_zfac(t->region->regiondata, t->center_global);
		}
		else {
			t->zfac = 0.0f;
		}
	}
}

ROSE_STATIC void transform_restore_element(TransData *td) {
	if (td->extension) {
		if (td->extension->size) {
			copy_v3_v3(td->extension->size, td->extension->isize);
		}
	}
}

ROSE_STATIC void transform_restore(TransInfo *t) {
	for (TransDataContainer *tc = t->data_container, *tc_end = t->data_container + t->data_container_length; tc != tc_end; tc++) {
		for (TransData *td = tc->data, *td_end = tc->data + tc->data_length; td != td_end; td++) {
			transform_restore_element(td);
		}

		unit_m3(t->mat);
	}

	ED_transform_data_recalc(t);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Public Transform API
 * \{ */

bool ED_transform_init(rContext *C, TransInfo *t, wmOperator *op, const wmEvent *event, eTransformMode mode) {
	PropertyRNA *property;

	mode = transform_mode_really_used(C, mode);

	t->context = C;

	eTransformContext options = CTX_NONE;

#if 0
	if ((property = RNA_struct_find_property(op->ptr, "xxx")) && RNA_property_is_set(op->ptr, property)) {
		if (RNA_property_boolean_get(op->ptr, property)) {
			options |= CTX_NONE;
		}
	}
#endif

	t->options = options;
	t->mode = mode;

	t->launch_event = event ? WM_userdef_event_type_from_keymap_type(event->type) : -1;

	if (t->launch_event == 0) {
		t->launch_event = LEFTMOUSE;
	}

	ED_transform_init_info(C, t, op, event);

	if (t->spacetype == SPACE_VIEW3D) {
		// t->draw_handle_view = ED_region_draw_cb_activate(...);
		// t->draw_handle_pixel = ED_region_draw_cb_activate(...);
		// t->draw_handle_cursor = ED_region_draw_cb_activate(...);
	}
	
	ED_transform_data_create(C, t); /* Make #TransData structs from selection. */

	if (t->data_length_all == 0) {
		ED_transform_post(C, t);
		return 0;
	}

	mode = t->mode;

	transform_center_calculate(t);

	if (event) {
		int mval[2];
		if (t->flag & T_EVENT_DRAG_START) {
			WM_event_drag_start_mval(event, t->region, mval);
		}
		else {
			copy_v2_v2_int(mval, event->mouse_local);
		}
		ED_mouse_input_init(t, &t->mouse, t->center2d, mval);
	}

	ED_transform_mode_init(t, op, mode);

	if (t->state == TRANS_CANCEL) {
		ED_transform_post(C, t);
		return 0;
	}

	/* Transformation axis from operator */
	if ((property = RNA_struct_find_property(op->ptr, "orient_axis")) && RNA_property_is_set(op->ptr, property)) {
		t->axis = RNA_property_enum_get(op->ptr, property);
	}

	t->context = NULL;

	return 1;
}

ROSE_INLINE void transform_custom_data_free(TransInfo *t, TransDataContainer *tc, TransCustomData *data) {
	if (data->free_cb) {
		data->free_cb(t, tc, data);
		ROSE_assert(data->data == NULL);
	}
	else if ((data->data != NULL) && data->use_free) {
		MEM_freeN(data->data);
		data->data = NULL;
	}
	/* In case modes are switched in the same transform session. */
	data->free_cb = NULL;
	data->use_free = false;
}

ROSE_INLINE void transform_custom_data_container_free(TransInfo *t, TransDataContainer *tc, TransCustomDataContainer *tcdc) {
	TransCustomData *data = &tcdc->first;
	for (int i = 0; i < sizeof(TransCustomDataContainer) / sizeof(TransCustomData); i++, data++) {
		transform_custom_data_free(t, tc, data);
	}
}

void ED_transform_post(rContext *C, TransInfo *t) {
	transform_custom_data_container_free(t, NULL, &t->custom);
	for (TransDataContainer *tc = t->data_container, *tc_end = t->data_container + t->data_container_length; tc != tc_end; tc++) {
		transform_custom_data_container_free(t, tc, &tc->custom);
	}

	/* #ED_transform_post can be called when nothing is selected, so data is NULL already */
	if (t->data_length_all != 0) {
		for (TransDataContainer *tc = t->data_container, *tc_end = t->data_container + t->data_container_length; tc != tc_end; tc++) {
			TransData *td = tc->data;
			for (int i = 0; i < tc->data_length; i++, td++) {
				// 
			}
			MEM_freeN(tc->data);

			MEM_SAFE_FREE(tc->data_extension);
		}
	}

	MEM_SAFE_FREE(t->data_container);
	t->data_container = NULL;
	
	if (t->mouse.data) {
		MEM_freeN(t->mouse.data);
	}
}

ROSE_STATIC bool transform_event_modal_constraint(TransInfo *t, int type) {
	int cur_constraint = -1;

	if (t->modifiers & (MOD_CONSTRAINT_SELECT_AXIS)) {
		t->modifiers &= ~MOD_CONSTRAINT_SELECT_AXIS;

		/* Avoid changing orientation in this case. */
		cur_constraint = -2;
	}
	else if (t->constraint.mode & CON_APPLY) {
		cur_constraint = t->constraint.mode & (CON_AXIS0 | CON_AXIS1 | CON_AXIS2);
	}

	int new_constraint = 0;

	switch (type) {
		case TFM_MODAL_AXIS_X: {
			new_constraint = CON_AXIS0;
		} break;
		case TFM_MODAL_AXIS_Y: {
			new_constraint = CON_AXIS1;
		} break;
		case TFM_MODAL_AXIS_Z: {
			new_constraint = CON_AXIS2;
		} break;
		default:
			/* Invalid key */
			return false;
	}

	if (cur_constraint != new_constraint) {
		transform_constraint_set(t, new_constraint, "");
	}
	else {
		transform_constraint_stop(t);
	}

	return true;
}

int ED_transform_event(TransInfo *t, const wmEvent *evt) {
	bool handled = false;

	if (evt->type == MOUSEMOVE) {
		copy_v2_v2_int(t->mval, evt->mouse_local);

		ED_mouse_input_apply(t, &t->mouse, t->mval, t->values);
		handled = true;
	}

	if (evt->type == EVT_MODAL_MAP) {
		switch (evt->value) {
			case TFM_MODAL_CANCEL: {
				t->state = TRANS_CANCEL;
				handled = true;
			} break;
			case TFM_MODAL_CONFIRM: {
				t->state = TRANS_CONFIRM;
				handled = true;
			} break;
			case TFM_MODAL_AXIS_X:
			case TFM_MODAL_AXIS_Y:
			case TFM_MODAL_AXIS_Z: {
				if (transform_event_modal_constraint(t, evt->value)) {
					handled = true;
				}
			} break;
		}
	}

	if (handled) {
		return 0;
	}

	return OPERATOR_PASS_THROUGH;
}

wmOperatorStatus ED_transform_end(rContext *C, TransInfo *t) {
	wmOperatorStatus ret = OPERATOR_RUNNING_MODAL;

	t->context = C;

	if (!ELEM(t->state, TRANS_STARTING, TRANS_RUNNING)) {
		/* handle restoring objects */
		if (t->state == TRANS_CANCEL) {
			transform_restore(t);
			ret = OPERATOR_CANCELLED;
		}
		else {
			ret = OPERATOR_FINISHED;
		}

		ED_transform_special_after_transform_update(C, t);

		ED_transform_post(C, t);
	}

	t->context = NULL;

	return ret;
}

void ED_transform_save(rContext *C, TransInfo *t, wmOperator *op) {
	PropertyRNA *property;
	/* Save back mode in case we're in the generic operator */
	if ((property = RNA_struct_find_property(op->ptr, "mode"))) {
		RNA_property_enum_set(op->ptr, property, t->mode);
	}
	if ((property = RNA_struct_find_property(op->ptr, "value"))) {
		if (RNA_property_array_check(property)) {
			RNA_property_float_set_array(op->ptr, property, t->values_final);
		}
		else {
			RNA_property_float_set(op->ptr, property, t->values_final[0]);
		}
	}
}

void ED_transform_apply(rContext *C, TransInfo *t) {
	t->context = C;

	if (t->transform) {
		t->transform(t, t->mval);
	}

	t->context = NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Private Transform Utilities
 * \{ */

int transform_moving_special(TransInfo *t) {
	if ((t->options & CTX_OBJECT) != 0) {
		return G_TRANSFORM_OBJ;
	}

	return 0;
}

void transform_view_vector_convert(TransInfo *t, float r_vec[3], double dx, double dy) {
	if ((t->spacetype == SPACE_VIEW3D) && (t->region->regiontype == RGN_TYPE_WINDOW)) {
		const float xy_delta[2] = {(float)dx, (float)dy};
		ED_view3d_win_to_delta(t->region, xy_delta, t->zfac, r_vec);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Keymap Modal Items
 *
 * \note these values are saved in key-map files, do not change then but just add new ones.
 * \{ */

ROSE_STATIC int transform_modal_item_poll(const wmOperator *op, int value) {
	const TransInfo *t = op->customdata;
	switch (value) {
		/* No-op */
	}
	return true;
}

struct wmKeyMap *transform_modal_keymap(struct wmKeyConfig *keyconf) {
	static const EnumPropertyItem modal_items[] = {
		{ .value = TFM_MODAL_CONFIRM, .identifier = "CONFIRM", .icon = 0, .name = "Confirm", .description = "" },
		{ .value = TFM_MODAL_CANCEL, .identifier = "CANCEL", .icon = 0, .name = "Cancel", .description = "" },
		{ .value = 0, .identifier = NULL, .icon = 0, .name = NULL, .description = NULL },
	};

	wmKeyMap *keymap = WM_modalkeymap_ensure(keyconf, "Transform Modal Map", modal_items);
	keymap->poll_modal_item = transform_modal_item_poll;

	/* clang-format off */

	WM_modalkeymap_add_item(keymap, &(const KeyMapItem_Params) {
		.type = EVT_SPACEKEY,
		.value = KM_PRESS,
		.modifier = KM_ANY,
	},
	TFM_MODAL_CONFIRM);

	WM_modalkeymap_add_item(keymap, &(const KeyMapItem_Params) {
		.type = EVT_RETKEY,
		.value = KM_PRESS,
		.modifier = KM_ANY,
	},
	TFM_MODAL_CONFIRM);

	WM_modalkeymap_add_item(keymap, &(const KeyMapItem_Params) {
		.type = EVT_PADENTER,
		.value = KM_PRESS,
		.modifier = KM_ANY,
	},
	TFM_MODAL_CONFIRM);

	WM_modalkeymap_add_item(keymap, &(const KeyMapItem_Params) {
		.type = LEFTMOUSE,
		.value = KM_PRESS,
		.modifier = KM_ANY,
	},
	TFM_MODAL_CONFIRM);

	WM_modalkeymap_add_item(keymap, &(const KeyMapItem_Params) {
		.type = EVT_ESCKEY,
		.value = KM_PRESS,
		.modifier = KM_ANY,
	},
	TFM_MODAL_CANCEL);

	WM_modalkeymap_add_item(keymap, &(const KeyMapItem_Params) {
		.type = RIGHTMOUSE,
		.value = KM_PRESS,
		.modifier = KM_ANY,
	},
	TFM_MODAL_CANCEL);

	WM_modalkeymap_add_item(keymap, &(const KeyMapItem_Params) {
		.type = EVT_XKEY,
		.value = KM_PRESS,
		.modifier = KM_ANY,
	},
	TFM_MODAL_AXIS_X);

	WM_modalkeymap_add_item(keymap, &(const KeyMapItem_Params) {
		.type = EVT_YKEY,
		.value = KM_PRESS,
		.modifier = KM_ANY,
	},
	TFM_MODAL_AXIS_Y);

	WM_modalkeymap_add_item(keymap, &(const KeyMapItem_Params) {
		.type = EVT_ZKEY,
		.value = KM_PRESS,
		.modifier = KM_ANY,
	},
	TFM_MODAL_AXIS_Z);

	/* clang-format on */

	return keymap;
}

/** \} */
