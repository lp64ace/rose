#include "MEM_guardedalloc.h"

#include "KER_context.h"
#include "KER_idprop.h"
#include "KER_global.h"
#include "KER_main.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_prototypes.h"

#include "WM_api.h"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name OperatorType
 * \{ */

ROSE_STATIC ListBase operator_types;

ROSE_INLINE wmOperatorType *wm_operatortype_append_begin(void) {
	wmOperatorType *ot = MEM_callocN(sizeof(wmOperatorType), "wmOperatorType");

	ot->srna = RNA_def_struct_ex(&ROSE_RNA, "", &RNA_OperatorProperties);

	return ot;
}

ROSE_INLINE void wm_operatortype_append_end(wmOperatorType *ot) {
	RNA_def_struct_ui_text(ot->srna, ot->name, ot->description);
	RNA_def_struct_identifier(&ROSE_RNA, ot->srna, ot->idname);

	LIB_addtail(&operator_types, ot);
}

void WM_operatortype_append(void (*opfunc)(wmOperatorType *ot)) {
	wmOperatorType *ot = wm_operatortype_append_begin();
	opfunc(ot);
	wm_operatortype_append_end(ot);
}

ROSE_INLINE void operatortype_ghash_free_cb(wmOperatorType *ot) {
	RNA_struct_free(&ROSE_RNA, ot->srna);
	MEM_freeN(ot);
}

void WM_operatortype_clear() {
	LISTBASE_FOREACH_MUTABLE(wmOperatorType *, ot, &operator_types) {
		operatortype_ghash_free_cb(ot);
	}
}

wmOperatorType *WM_operatortype_find(const char *idname, bool quiet) {
	LISTBASE_FOREACH(wmOperatorType *, ot, &operator_types) {
		if (STREQ(idname, ot->idname)) {
			return ot;
		}
	}

	if (!quiet) {
		fprintf(stderr, "[WindowManager] Search for unkown operator \"%s\".", idname);
	}

	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operator
 * \{ */

bool WM_operator_poll(rContext *C, wmOperatorType *ot) {
	if (ot->poll) {
		return ot->poll(C);
	}

	return true;
}

void WM_operator_properties_create_ptr(PointerRNA *ptr, wmOperatorType *ot) {
	/* Set the ID so the context can be accessed: see #STRUCT_NO_CONTEXT_WITHOUT_OWNER_ID. */
	*ptr = RNA_pointer_create_discrete((ID *)(G_MAIN->wm.first), ot->srna, NULL);
}

void WM_operator_properties_create(PointerRNA *ptr, const char *opstring) {
	wmOperatorType *ot = WM_operatortype_find(opstring, false);

	if (ot) {
		WM_operator_properties_create_ptr(ptr, ot);
	}
}

void WM_operator_properties_alloc(PointerRNA **ptr, IDProperty **properties, const char *opstring) {
	IDProperty *tmp_properties = NULL;
	/* Allow passing nullptr for properties, just create the properties here then. */
	if (properties == NULL) {
		properties = &tmp_properties;
	}

	if (*properties == NULL) {
		*properties = IDP_New(IDP_GROUP, NULL, 0, "wmOpItemProp", 0);
	}

	if (*ptr == NULL) {
		*ptr = MEM_callocN(sizeof(PointerRNA), "wmOpItemPtr");
		WM_operator_properties_create(*ptr, opstring);
	}

	(*ptr)->data = *properties;
}

void WM_operator_properties_free(PointerRNA *ptr) {
	IDProperty *properties = (IDProperty *)(ptr->data);

	if (properties) {
		IDP_FreeProperty(properties);
		ptr->data = NULL; /* Just in case. */
	}
}

void WM_operator_free(wmOperator *op) {
	if (op->ptr) {
		op->properties = (IDProperty *)(op->ptr->data);
		MEM_freeN(op->ptr);
	}

	if (op->properties) {
		IDP_FreeProperty(op->properties);
	}

	MEM_SAFE_FREE(op->customdata);
	MEM_freeN(op);
}

/** \} */
