#include "MEM_guardedalloc.h"

#include "DNA_view3d_types.h"

#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "KER_context.h"
#include "KER_scene.h"
#include "KER_object.h"

#include "WM_api.h"

#include "view3d_navigate.h"

ROSE_INLINE void viewops_data_init_navigation(rContext *C, ViewOpsData *vod, const wmEvent *event, const ViewOpsType *nav_type) {
	if (event) {
		vod->event_type = event->type;

		copy_v2_v2_int(vod->init_xy, event->mouse_xy);
		copy_v2_v2_int(vod->prev_xy, event->mouse_xy);
	}

	RegionView3D *rv3d = (RegionView3D *)vod->region->regiondata;

	if (rv3d) {
		copy_qt_qt(vod->initial.viewquat, rv3d->viewquat);
		copy_qt_qt(vod->current.viewquat, rv3d->viewquat);

		copy_v3_v3(vod->current.viewloc, rv3d->viewloc);
		copy_v3_v3(vod->current.viewloc, rv3d->viewloc);
	}

	vod->nav_type = nav_type;
}

ROSE_INLINE void viewops_data_free(rContext *C, ViewOpsData *vod) {
	if (vod) {
		MEM_freeN(vod);
	}
}

ROSE_STATIC wmOperatorStatus view3d_navigation_invoke_generic(rContext *C, ViewOpsData *vod, const wmEvent *event, PointerRNA *ptr, const ViewOpsType *nav_type) {
	PropertyRNA *property;

	if (!nav_type->init_fn) {
		return OPERATOR_CANCELLED;
	}

	viewops_data_init_navigation(C, vod, event, nav_type);

	return nav_type->init_fn(C, vod, event, ptr);
}

wmOperatorStatus view3d_navigate_invoke_impl(rContext *C, wmOperator *op, const wmEvent *event, const ViewOpsType *nav_type) {
	ViewOpsData *vod = MEM_mallocN(sizeof(ViewOpsData), "ViewOpsData");
	
	vod->scene = CTX_data_scene(C);
	vod->region = CTX_wm_region(C);

	wmOperatorStatus ret = view3d_navigation_invoke_generic(C, vod, event, op->ptr, nav_type);
	op->customdata = (void *)vod;

	if (ret == OPERATOR_RUNNING_MODAL) {
		WM_event_add_modal_handler(C, op);
		return OPERATOR_RUNNING_MODAL;
	}

	viewops_data_free(C, vod);
	op->customdata = NULL;
	return ret;
}

ROSE_INLINE eV3D_OpEvent view3d_navigate_event(ViewOpsData *vod, const wmEvent *event) {
	if (event->type == vod->event_type && (event->value == KM_RELEASE || event->type == MOUSEMOVE)) {
		return VIEW_CONFIRM;
	}
	if (event->type == MOUSEMOVE) {
		return VIEW_APPLY;
	}
	if (event->type == EVT_ESCKEY && event->value == KM_PRESS) {
		return VIEW_CANCEL;
	}

	return VIEW_PASS;
}

wmOperatorStatus view3d_navigate_modal_fn(rContext *C, wmOperator *op, const wmEvent *event) {
	ViewOpsData *vod = (ViewOpsData *)(op->customdata);

	const eV3D_OpEvent event_code = view3d_navigate_event(vod, event);

	wmOperatorStatus ret = vod->nav_type->apply_fn(C, vod, event_code, event->mouse_xy);
	if ((ret & OPERATOR_CANCELLED) != 0) {
		view3d_navigate_cancel_fn(C, op);
	}

	if ((ret & OPERATOR_RUNNING_MODAL) == 0) {
		viewops_data_free(C, vod);
		op->customdata = NULL;
	}

	return ret;
}

void view3d_navigate_cancel_fn(rContext *C, wmOperator *op) {
	ViewOpsData *vod = (ViewOpsData *)(op->customdata);

	RegionView3D *rv3d = (RegionView3D *)vod->region->regiondata;
	if (rv3d) {
		copy_qt_qt(rv3d->viewquat, vod->initial.viewquat);
		copy_qt_qt(rv3d->viewloc, vod->initial.viewloc);
	}
}
