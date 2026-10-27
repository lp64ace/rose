#include "MEM_guardedalloc.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "LIB_assert.h"
#include "LIB_math_vector.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_global.h"

#include "ED_screen.h"
#include "ED_transform.h"

#include "WM_api.h"

#include "transform.h"

typedef struct TransformModeItem {
	const char *idname;
	int mode;
	void (*opfunc)(wmOperatorType *);
} TransformModeItem;

static const char OP_TRANSLATION[] = "TRANSFORM_OT_translate";
static const char OP_ROTATION[] = "TRANSFORM_OT_rotate";
static const char OP_RESIZE[] = "TRANSFORM_OT_resize";

const EnumPropertyItem rna_enum_transform_mode_types[] = {
	{.value = TFM_INIT, .identifier = "INIT", .icon = 0, .name = "Init", .description = ""},
	{.value = TFM_NONE, .identifier = "NONE", .icon = 0, .name = "None", .description = ""},  // Dummy
	{.value = TFM_TRANSLATION, .identifier = "TRANSLATION", .icon = 0, .name = "Translation", .description = ""},
	{.value = TFM_ROTATION, .identifier = "ROTATION", .icon = 0, .name = "Rotation", .description = ""},
	{.value = TFM_RESIZE, .identifier = "RESIZE", .icon = 0, .name = "Resize", .description = ""},
	{.value = 0, .identifier = NULL, .icon = 0, .name = NULL, .description = NULL},
};

void TRANSFORM_OT_translate(struct wmOperatorType *ot);
void TRANSFORM_OT_rotate(struct wmOperatorType *ot);
void TRANSFORM_OT_resize(struct wmOperatorType *ot);
void TRANSFORM_OT_transform(struct wmOperatorType *ot);

static const TransformModeItem transform_mode_items[] = {
	{.idname = OP_TRANSLATION, .mode = TFM_TRANSLATION, .opfunc = TRANSFORM_OT_translate},
	{.idname = OP_ROTATION, .mode = TFM_ROTATION, .opfunc = TRANSFORM_OT_rotate},
	{.idname = OP_RESIZE, .mode = TFM_RESIZE, .opfunc = TRANSFORM_OT_resize},
};

ROSE_STATIC eTransformMode transform_operator_mode(wmOperator *op) {
	for (TransformModeItem *tmode = transform_mode_items; tmode->idname; tmode++) {
		if (op->type->idname == tmode->idname) {
			return tmode->mode;
		}
	}

	return (eTransformMode)RNA_enum_get(op->ptr, "mode");
}

ROSE_STATIC bool transform_operator_data(rContext *C, wmOperator *op, const wmEvent *evt) {
	bool success = true;
	if (op->customdata == NULL) {
		TransInfo *ptr = MEM_callocN(sizeof(TransInfo), "transform_operator_data::TransInfo");

		eTransformMode mode = transform_operator_mode(op);
		if ((success &= ED_transform_init(C, ptr, op, evt, mode))) {
			G.moving = transform_moving_special(ptr);
			op->customdata = ptr;
		}
		else {
			/** Failed to init the transform data, free the pointer! */
			MEM_freeN(ptr);
		}
	}

	return success;
}

ROSE_STATIC void transform_operator_exit(rContext *C, wmOperator *op) {
	ED_transform_save(C, (TransInfo *)op->customdata, op);
	MEM_freeN(op->customdata);
	op->customdata = NULL;
	G.moving = 0;
}

ROSE_STATIC wmOperatorStatus transform_exec(rContext *C, wmOperator *op) {
	if (!transform_operator_data(C, op, NULL)) {
		G.moving = 0;
		return OPERATOR_CANCELLED;
	}

	TransInfo *info = op->customdata;
	ED_transform_apply(C, info);
	ED_transform_end(C, info);

	transform_operator_exit(C, op);
	return OPERATOR_FINISHED;
}

ROSE_STATIC wmOperatorStatus transform_invoke(rContext *C, wmOperator *op, const wmEvent *evt) {
	if (!transform_operator_data(C, op, evt)) {
		G.moving = 0;
		return OPERATOR_CANCELLED;
	}

	/* When modal, allow 'value' to set initial offset. */
	if ((evt == NULL) && RNA_struct_property_is_set(op->ptr, "value")) {
		return transform_exec(C, op);
	}

	/* add temp handler */
	WM_event_add_modal_handler(C, op);

	/* Use when modal input has some transformation to begin with. */
	TransInfo *info = op->customdata;
	if (!is_zero_v4(info->values_modal_offset)) {
		ED_transform_apply(C, info);
	}

	return OPERATOR_RUNNING_MODAL;
}

ROSE_STATIC wmOperatorStatus transform_modal(rContext *C, wmOperator *op, const wmEvent *event) {
	TransInfo *info = op->customdata;
	const eTransformMode mode = info->mode;

	wmOperatorStatus ret = 0;

	/* XXX insert keys are called here, and require context */
	info->context = C;
	ret = ED_transform_event(C, info, event);
	info->context = NULL;

	/**
	 * XXX, workaround: active needs to be calculated before transforming,
	 * since we're not reading from 'td->center' in this case. see: T40241
	 */
	ED_transform_apply(C, info);

	ret |= ED_transform_end(C, info);

	if ((ret & OPERATOR_RUNNING_MODAL) == 0) {
		transform_operator_exit(C, op);
		ret &= ~OPERATOR_PASS_THROUGH; /* Preventively remove pass-through. */
	}
	else {
		if (mode != info->mode) {
			/**
			 * WARNING: this is not normal to switch operator types
			 * normally it would not be supported but transform happens
			 * to share callbacks between different operators.
			 */
			wmOperatorType *ot = NULL;
			TransformModeItem *item = transform_mode_items;
			while (item->idname) {
				if (item->mode == info->mode) {
					ot = WM_operatortype_find(item->idname, false);
					break;
				}
				item++;
			}

			ROSE_assert(ot != NULL);
			if (ot) {
				WM_operator_type_set(op, ot);
			}
		}
	}

	return ret;
}

ROSE_STATIC void transform_cancel(rContext *C, wmOperator *op) {
	TransInfo *info = (TransInfo *)op->customdata;

	info->state = TRANS_CANCEL;
	ED_transform_end(C, info);
	transform_operator_exit(C, op);
}

void TRANSFORM_OT_transform(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Transform";
	ot->description = "Transform selected items by mode type";
	ot->idname = "TRANSFORM_OT_transform";
	ot->flag = OPTYPE_BLOCKING;

	/* api callbacks */
	ot->invoke = transform_invoke;
	ot->exec = transform_exec;
	ot->modal = transform_modal;
	ot->cancel = transform_cancel;
	ot->poll = ED_operator_screen_active;

	RNA_def_enum(ot->srna, "mode", rna_enum_transform_mode_types, TFM_TRANSLATION, "Mode", "");
	RNA_def_float_vector(ot->srna, "value", 4, NULL, -FLT_MAX, FLT_MAX, "Values", "", -FLT_MAX, FLT_MAX);
}
