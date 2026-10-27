#include "KER_context.h"
#include "KER_global.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "WM_api.h"
#include "WM_event.h"

#include "transform.h"

/* -------------------------------------------------------------------- */
/** \name Transform API
 * \{ */

ROSE_INLINE eTransformMode transform_mode_really_used(rContext *C, eTransformMode mode) {

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

	ED_transform_center_calculate(t);

	if (event) {
		/* Initialize accurate transform to settings requested by keymap. */
		bool use_accurate = false;

		if ((property = RNA_struct_find_property(op->ptr, "use_accurate")) && RNA_property_is_set(op->ptr, property)) {
			if (RNA_property_boolean_get(op->ptr, property)) {
				use_accurate = true;
			}
		}

		int mval[2];
		if (t->flag & T_EVENT_DRAG_START) {
			WM_event_drag_start_mval(event, t->region, mval);
		}
		else {
			copy_v2_v2_int(mval, event->mval);
		}
		ED_mouse_input_init(t, &t->mouse, t->center2d, mval, use_accurate);
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

/** \} */
