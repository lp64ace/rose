#include "MEM_guardedalloc.h"

#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_vector_types.h"
#include "DNA_userdef_types.h"
#include "DNA_view3d_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

#include "DRW_engine.h"

#include "ED_screen.h"

#include "LIB_assert.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_global.h"
#include "KER_layer.h"
#include "KER_screen.h"
#include "KER_scene.h"
#include "KER_object.h"

#include "GPU_framebuffer.h"
#include "GPU_matrix.h"
#include "GPU_select.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_window.h"

#include "view3d_intern.h"
#include "view3d_navigate.h"

#include <stdio.h>

/**
 * The default value for the maximum number of elements that can be selected at once
 * using view-port selection.
 *
 * \note in many cases this defines the size of fixed-size stack buffers,
 * so take care increasing this value.
 */
#define MAXPICKELEMS 2500

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
ROSE_INLINE bool view3d_rotation_poll(rContext *C) {
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

ROSE_INLINE void viewpan_apply(ViewOpsData *vod, const int event_xy[2]) {
	RegionView3D *rv3d = (RegionView3D *)vod->region->regiondata;

	if (true) {
		const float sensitivity = U.view_rotate_sensitivity_turntable / PIXELSIZE;

		/* Camera forward vector (Z axis in camera space) */
		float dX[3], dY[3];
		float mat[3][3];

		quat_to_mat3(mat, vod->current.viewquat);

		mul_v3_v3fl(dX, mat[0], sensitivity * -(event_xy[0] - vod->prev_xy[0]));
		mul_v3_v3fl(dY, mat[1], sensitivity * -(event_xy[1] - vod->prev_xy[1]));

		add_v3_v3(vod->current.viewloc, dX);
		add_v3_v3(vod->current.viewloc, dY);
	}

	/**
	 * use a working copy so view location locking doesn't overwrite the locked
	 * location back into the view we calculate with
	 */
	copy_v3_v3(rv3d->viewloc, vod->current.viewloc);

	vod->prev_xy[0] = event_xy[0];
	vod->prev_xy[1] = event_xy[1];

	ED_region_tag_redraw(vod->region);
}

/**
 * Return's false if we should deny move to the user of the View3D!
 */
ROSE_INLINE bool view3d_pan_poll(rContext *C) {
	return true;
}

ROSE_INLINE wmOperatorStatus viewpan_invoke_impl(rContext *C, ViewOpsData *vod, const wmEvent *event, PointerRNA *ptr) {
	eV3D_OpEvent event_code = ELEM(event->type, MOUSEROTATE, MOUSEPAN) ? VIEW_CONFIRM : VIEW_PASS;

	if (event_code == VIEW_CONFIRM) {
		viewpan_apply(vod, event->mouse_xy);

		return OPERATOR_FINISHED;
	}

	return OPERATOR_RUNNING_MODAL;
}

ROSE_INLINE wmOperatorStatus viewpan_modal_impl(rContext *C, ViewOpsData *vod, const eV3D_OpEvent event_code, const int xy[2]) {
	if (ELEM(event_code, VIEW_APPLY, VIEW_CONFIRM)) {
		viewpan_apply(vod, xy);
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

const ViewOpsType ViewOpsType_pan = {
	.flag = VIEWOPS_FLAG_NONE,
	.idname = "VIEW3D_OT_pan",
	.poll_fn = view3d_pan_poll,
	.init_fn = viewpan_invoke_impl,
	.apply_fn = viewpan_modal_impl,
};

static wmOperatorStatus viewpan_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	return view3d_navigate_invoke_impl(C, op, event, &ViewOpsType_pan);
}

static void VIEW3D_OT_pan(wmOperatorType *ot) {
/* identifiers */
	ot->name = "Pan View";
	ot->description = "Pan the view";
	ot->idname = ViewOpsType_pan.idname;

	/* API callbacks. */
	ot->invoke = viewpan_invoke;
	ot->modal = view3d_navigate_modal_fn;
	ot->poll = view3d_pan_poll;
	ot->cancel = view3d_navigate_cancel_fn;

	/* flags */
	ot->flag = 0;
}

ROSE_INLINE void viewzoom_apply(ViewOpsData *vod, int delta) {
	RegionView3D *rv3d = (RegionView3D *)vod->region->regiondata;

	if (true) {
		const float sensitivity = U.view_rotate_sensitivity_turntable / PIXELSIZE;

		float dZ[3];
		float mat[3][3];

		quat_to_mat3(mat, vod->current.viewquat);

		mul_v3_v3fl(dZ, mat[2], sensitivity * -delta);

		add_v3_v3(vod->current.viewloc, dZ);
	}

	/**
	 * use a working copy so view location locking doesn't overwrite the locked
	 * location back into the view we calculate with
	 */
	copy_v3_v3(rv3d->viewloc, vod->current.viewloc);

	ED_region_tag_redraw(vod->region);
}

/**
 * Return's false if we should deny zoom to the user of the View3D!
 */
ROSE_INLINE bool view3d_zoom_poll(rContext *C) {
	return true;
}

ROSE_INLINE wmOperatorStatus viewzoom_invoke_impl(rContext *C, ViewOpsData *vod, const wmEvent *event, PointerRNA *ptr) {
	int xy[2];

	PropertyRNA *property = RNA_struct_find_property(ptr, "delta");

	const int delta = RNA_property_is_set(ptr, property) ? RNA_property_int_get(ptr, property) : 0;
	if (delta) {
		viewzoom_apply(vod, delta);
		return OPERATOR_FINISHED;
	}

	ROSE_assert_unreachable();

	return OPERATOR_RUNNING_MODAL;
}

ROSE_INLINE wmOperatorStatus viewzoom_modal_impl(rContext *C, ViewOpsData *vod, const eV3D_OpEvent event_code, const int xy[2]) {
	return OPERATOR_FINISHED;
}

const ViewOpsType ViewOpsType_zoom = {
	.flag = VIEWOPS_FLAG_NONE,
	.idname = "VIEW3D_OT_zoom",
	.poll_fn = view3d_zoom_poll,
	.init_fn = viewzoom_invoke_impl,
	.apply_fn = viewzoom_modal_impl,
};

ROSE_INLINE wmOperatorStatus viewzoom_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	/* Near duplicate logic in #viewdolly_invoke(), changes here may apply there too. */
	return view3d_navigate_invoke_impl(C, op, event, &ViewOpsType_zoom);
}

ROSE_INLINE wmOperatorStatus viewzoom_exec(rContext *C, wmOperator *op) {
	return OPERATOR_RUNNING_MODAL;
}

static void VIEW3D_OT_zoom(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Zoom View";
	ot->description = "Zoom in/out in the view";
	ot->idname = ViewOpsType_zoom.idname;

	/* API callbacks. */
	ot->invoke = viewzoom_invoke;
	ot->exec = viewzoom_exec;
	ot->modal = view3d_navigate_modal_fn;
	ot->poll = view3d_zoom_poll;
	ot->cancel = view3d_navigate_cancel_fn;

	/* rna */
	RNA_def_int(ot->srna, "delta", 0, INT_MIN, INT_MAX, "Delta", "", INT_MIN, INT_MAX);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Select
 * \{ */

ROSE_INLINE void view3d_region_requires_gpu_context(wmWindow *window, ARegion *region) {
	if ((region == NULL) || (region->regiontype != RGN_TYPE_WINDOW)) {
		fprintf(stderr, "[Editors] #%s failed, wrong region\n", __func__);
	}
	else {
		RegionView3D *rv3d = region->regiondata;

		ED_region_pixelspace(region);

		GPU_matrix_projection_set(rv3d->winmat);
		GPU_matrix_set(rv3d->viewmat);
	}
}

ROSE_INLINE void view3d_operator_requires_gpu_context(rContext *C) {
	wmWindow *window = CTX_wm_window(C);
	ARegion *region = CTX_wm_region(C);

	view3d_region_requires_gpu_context(window, region);
}

typedef struct DrawSelectLoopUserData {
	int select;
	int pass;
	int hits;
	GPUSelectResult *buffer;
	size_t maxlen;
	const rcti *rect;
} DrawSelectLoopUserData;

ROSE_INLINE bool view3d_select_pass(int stage, void *userdata) {
	DrawSelectLoopUserData *data = (DrawSelectLoopUserData *)userdata;

	bool need_another_pass = false;

	if (stage == DRW_SELECT_PASS_PRE) {
		GPU_select_begin(data->buffer, data->maxlen, data->rect, data->select, data->hits);
		/* always run POST after PRE. */
		need_another_pass |= true;
	}
	else if (stage == DRW_SELECT_PASS_POST) {
		int hits = GPU_select_end();
		if (data->pass == 0) {
			/* quirk of GPU_select_end, only take hits value from first call. */
			data->hits = hits;
		}
		if (data->select == GPU_SELECT_NEAREST_FIRST_PASS) {
			data->select = GPU_SELECT_NEAREST_SECOND_PASS;
			need_another_pass |= (hits > 0);
		}
		data->pass += 1;
	}

	return need_another_pass;
}
ROSE_INLINE bool view3d_object_filter(struct Object *ob, void *user_data) {
	return true;
}

ROSE_INLINE void view3d_select_buffer_cache_init(View3D *v3d, ViewLayer *layer) {
	size_t bases_length = 0;
	Base **bases = KER_view_layer_array_from_bases(layer, v3d, &bases_length);
	DRW_select_buffer_context_create(bases, bases_length);
	MEM_freeN(bases);
}

ROSE_INLINE void view3d_select_buffer_cache_init_with_generic_userdata(void *userdata, View3D *v3d, ViewLayer *layer) {
	view3d_select_buffer_cache_init(v3d, layer);
	UNUSED_VARS(userdata);
}

ROSE_INLINE int view3d_gpu_select_ex(rContext *C, GPUSelectResult *buffer, size_t maxlen, Depsgraph *depsgraph, const rcti *rect) {
	ViewLayer *layer = DEG_get_evaluated_view_layer(depsgraph);
	ARegion *region = CTX_wm_region(C);
	View3D *v3d = CTX_wm_space_view3d(C);
	RegionView3D *rv3d = region->regiondata;

	view3d_select_buffer_cache_init_with_generic_userdata(NULL, v3d, layer);

	G.flag |= G_FLAG_PICKSEL;

	/* Re-use cache (rect must be smaller than the cached)
	 * other context is assumed to be unchanged */
	// if (GPU_select_is_cached()) {
	// 	GPU_select_begin(buffer, maxlen, &rect, GPU_SELECT_PICK_NEAREST, 0);
	// 	GPU_select_cache_load_id();
	// 	GPU_select_end();
	// }

	DRW_render_context_enable(true);

	GPU_matrix_push();
	GPU_matrix_identity_set();
	GPU_matrix_push_projection();
	GPU_matrix_identity_projection_set();

	ED_view3d_draw_setup_view(region, NULL, NULL, rect);

	/* We need to call "GPU_select_*" API's inside DRW_draw_select_loop
	 * because the OpenGL context created & destroyed inside this function. */
	struct DrawSelectLoopUserData drw_select_loop_user_data = {
		.pass = 0,
		.hits = 0,
		.buffer = buffer,
		.maxlen = maxlen,
		.rect = rect,
		.select = GPU_SELECT_PICK_NEAREST,
	};
	
	DRW_draw_select_loop(depsgraph, region, v3d, rect, view3d_select_pass, &drw_select_loop_user_data, view3d_object_filter, NULL);

	GPU_matrix_pop_projection();
	GPU_matrix_pop();

	DRW_render_context_disable(true);

	G.flag &= ~G_FLAG_PICKSEL;

	ED_view3d_draw_setup_view(region, NULL, NULL, NULL);

	return drw_select_loop_user_data.hits;
}

int ED_object_select_pick(rContext *C, GPUSelectResult *buffer, size_t maxlen, int x, int y, int radius) {
	Depsgraph *depsgraph = CTX_data_ensure_evaluated_depsgraph(C);

	rcti rect;
	LIB_rcti_init_pt_radius(&rect, x, y, radius);

	view3d_operator_requires_gpu_context(C);

	int hits = view3d_gpu_select_ex(C, buffer, maxlen, depsgraph, &rect);

	struct WindowManager *wm = CTX_wm_manager(C);

	/**
	 * Since #view3d_gpu_select_ex usually ends up doing nasty GPU operations we need to 
	 * restore the WindowManager's GPU context in order to continue rendering the UI/handlers.
	 */
	wm_window_reset_drawable(wm);

	return hits;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Select Operators
 * \{ */

ROSE_INLINE wmOperatorStatus view3d_select_exec(rContext *C, wmOperator *op);

ROSE_INLINE wmOperatorStatus view3d_select_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	RNA_int_set(op->ptr, "x", event->mouse_local[0]);
	RNA_int_set(op->ptr, "y", event->mouse_local[1]);

	return view3d_select_exec(C, op);
}

ROSE_INLINE wmOperatorStatus view3d_select_exec(rContext *C, wmOperator *op) {
	Scene *scene = CTX_data_scene(C);

	int x = RNA_int_get(op->ptr, "x");
	int y = RNA_int_get(op->ptr, "y");
	int radius = RNA_int_get(op->ptr, "radius");

	KER_object_update_select_id(CTX_data_main(C));

	const size_t maxlen = MAXPICKELEMS;

	GPUSelectResult *buffer = MEM_mallocN(maxlen * sizeof(GPUSelectResult), "SelectionBuffer");

	int hits = ED_object_select_pick(C, buffer, maxlen, x, y, radius);

	fprintf(stdout, "Hits : %d\n", hits);

	if (hits > 0) {
		for (const GPUSelectResult *buf_iter = buffer, *buf_end = buf_iter + hits; buf_iter < buf_end; buf_iter++) {
			if (buf_iter->depth == 0xffffffffu) {
				continue;
			}
			fprintf(stdout, "Base ID : %d\n", buf_iter->id);
		}
	}

	MEM_freeN(buffer);

	return OPERATOR_PASS_THROUGH | OPERATOR_FINISHED;
}

/**
 * Return's false if we should deny select to the user!
 */
ROSE_INLINE bool view3d_select_poll(rContext *C) {
	return CTX_wm_space_view3d(C) != NULL;
}

void VIEW3D_OT_select(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Select";
	ot->description = "Select and activate item(s)";
	ot->idname = "VIEW3D_OT_select";

	/* API callbacks. */
	ot->invoke = view3d_select_invoke;
	ot->exec = view3d_select_exec;
	ot->poll = view3d_select_poll;

	/* rna */
	RNA_def_int(ot->srna, "x", 0, INT_MIN, INT_MAX, "X", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "y", 0, INT_MIN, INT_MAX, "Y", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "radius", 1, INT_MIN, INT_MAX, "Radius", "", INT_MIN, INT_MAX);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Assigning Operator Types
 * \{ */

void view3d_operatortypes() {
	WM_operatortype_append(VIEW3D_OT_rotate);
	WM_operatortype_append(VIEW3D_OT_pan);
	WM_operatortype_append(VIEW3D_OT_zoom);
	WM_operatortype_append(VIEW3D_OT_select);
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

	WM_keymap_add_item(keymap, "VIEW3D_OT_pan", &(KeyMapItem_Params){
		.type = RIGHTMOUSE,
		.value = KM_PRESS,
		.modifier = KM_SHIFT,
	});

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "VIEW3D_OT_zoom", &(KeyMapItem_Params){
			.type = WHEELUPMOUSE,
			.value = KM_PRESS,
			.modifier = KM_NOTHING,
		});

		RNA_int_set(kmi->ptr, "delta", 40);
	} while(false);

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "VIEW3D_OT_zoom", &(KeyMapItem_Params){
			.type = EVT_PLUSKEY,
			.value = KM_PRESS,
			.modifier = KM_NOTHING,
		});

		RNA_int_set(kmi->ptr, "delta", 40);
	} while(false);

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "VIEW3D_OT_zoom", &(KeyMapItem_Params){
			.type = WHEELDOWNMOUSE,
			.value = KM_PRESS,
			.modifier = KM_NOTHING,
		});

		RNA_int_set(kmi->ptr, "delta", -40);
	} while(false);

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "VIEW3D_OT_zoom", &(KeyMapItem_Params){
			.type = EVT_MINUSKEY,
			.value = KM_PRESS,
			.modifier = KM_NOTHING,
		});

		RNA_int_set(kmi->ptr, "delta", -40);
	} while(false);

	do {
		wmKeyMapItem *kmi = WM_keymap_add_item(keymap, "VIEW3D_OT_select", &(KeyMapItem_Params){
			.type = LEFTMOUSE,
			.value = KM_PRESS,
			.modifier = KM_NOTHING,
		});
	} while(false);

	/* clang-format on */
}

/** \} */
