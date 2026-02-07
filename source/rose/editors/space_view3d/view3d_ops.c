#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_vector_types.h"
#include "DNA_userdef_types.h"
#include "DNA_view3d_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "ED_screen.h"

#include "LIB_assert.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_screen.h"
#include "KER_scene.h"
#include "KER_object.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_window.h"

#include "view3d_navigate.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Navigate
 * \{ */

ROSE_INLINE void viewrotate_apply(ViewOpsData *vod, const int event_xy[2]) {
	RegionView3D *rv3d = (RegionView3D *)vod->region->regiondata;

	if (true) {
		const float sensitivity = U.view_rotate_sensitivity_turntable / PIXELSIZE;

		/* Camera forward vector (Z axis in camera space) */
		float forward[3];
		float mat[3][3];

		quat_to_mat3(mat, vod->current.viewquat);

		copy_v3_v3(forward, mat[2]);
		float yaw = atan2f(forward[0], forward[2]) + sensitivity * -(event_xy[0] - vod->prev_xy[0]);
		float pitch = -asinf(forward[1]) + sensitivity * (event_xy[1] - vod->prev_xy[1]);

		/**
		 * Do NOT allow reversing the camera UP vector.
		 */
		CLAMP(pitch, -M_PI_2 + 1e-3f, M_PI_2 - 1e-3f);

		float q_yaw[4], q_pitch[4];
		axis_angle_to_quat_single(q_yaw, 'Y', yaw);
		axis_angle_to_quat_single(q_pitch, 'X', pitch);
		mul_qt_qtqt(vod->current.viewquat, q_yaw, q_pitch);
	}

	/* avoid precision loss over time */
	normalize_qt(vod->current.viewquat);

	/**
	 * use a working copy so view rotation locking doesn't overwrite the locked
	 * rotation back into the view we calculate with
	 */
	copy_qt_qt(rv3d->viewquat, vod->current.viewquat);

	vod->prev_xy[0] = event_xy[0];
	vod->prev_xy[1] = event_xy[1];

	ED_region_tag_redraw(vod->region);
}

/**
 * Return's false if we should deny rotation to the user of the View3D!
 */
ROSE_INLINE int view3d_rotation_poll(rContext *C) {
	return true;
}

ROSE_INLINE wmOperatorStatus viewrotate_invoke_impl(rContext *C, ViewOpsData *vod, const wmEvent *event, PointerRNA *ptr) {
	eV3D_OpEvent event_code = ELEM(event->type, MOUSEROTATE, MOUSEPAN) ? VIEW_CONFIRM : VIEW_PASS;

	if (event_code == VIEW_CONFIRM) {
		viewrotate_apply(vod, event->mouse_xy);

		return OPERATOR_FINISHED;
	}

	return OPERATOR_RUNNING_MODAL;
}

ROSE_INLINE wmOperatorStatus viewrotate_modal_impl(rContext *C, ViewOpsData *vod, const eV3D_OpEvent event_code, const int xy[2]) {
	if (ELEM(event_code, VIEW_APPLY, VIEW_CONFIRM)) {
		viewrotate_apply(vod, xy);
	}

	switch (event_code) {
		case VIEW_CONFIRM: {
			return OPERATOR_FINISHED;
		} break;
		case VIEW_CANCEL: {
			return OPERATOR_CANCELLED;
		} break;
	}

	return OPERATOR_RUNNING_MODAL;
}

const ViewOpsType ViewOpsType_rotate = {
	.flag = VIEWOPS_FLAG_NONE,
	.idname = "VIEW3D_OT_rotate",
	.poll_fn = view3d_rotation_poll,
	.init_fn = viewrotate_invoke_impl,
	.apply_fn = viewrotate_modal_impl,
};

static wmOperatorStatus viewrotate_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	return view3d_navigate_invoke_impl(C, op, event, &ViewOpsType_rotate);
}

static void VIEW3D_OT_rotate(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Rotate View";
	ot->description = "Rotate the view";
	ot->idname = ViewOpsType_rotate.idname;

	/* API callbacks. */
	ot->invoke = viewrotate_invoke;
	ot->modal = view3d_navigate_modal_fn;
	ot->poll = view3d_rotation_poll;
	ot->cancel = view3d_navigate_cancel_fn;

	/* flags */
	ot->flag = 0;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Assigning Operator Types
 * \{ */

void view3d_operatortypes() {
	WM_operatortype_append(VIEW3D_OT_rotate);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operator Key Map
 * \{ */

void view3d_keymap(wmKeyConfig *keyconf) {
	/* 3D View ------------------------------------------------ */
	wmKeyMap *keymap = WM_keymap_ensure(keyconf, "3D View", SPACE_VIEW3D, RGN_TYPE_WINDOW);

	/* clang-format off */

	WM_keymap_add_item(keymap, "VIEW3D_OT_rotate", &(KeyMapItem_Params){
		.type = RIGHTMOUSE,
		.value = KM_PRESS,
		.modifier = KM_NOTHING,
	});

	/* clang-format on */
}

/** \} */
