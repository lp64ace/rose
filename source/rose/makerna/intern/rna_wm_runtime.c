#include "DNA_windowmanager_types.h"

#include "WM_api.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_prototypes.h"

#include "rna_internal_types.h"
#include "rna_internal.h"

/* -------------------------------------------------------------------- */
/** \name WM Exposed Functions
 * \{ */

struct IDProperty **rna_OperatorProperties_idprops(PointerRNA *ptr) {
	return (struct IDProperty **)&ptr->data;
}

ROSE_INLINE wmOperator *rna_OperatorProperties_find_operator(PointerRNA *ptr) {
	if (ptr->owner == NULL || GS(ptr->owner->name) != ID_WM) {
		return NULL;
	}

	WindowManager *wm = (WindowManager *)ptr->owner;
	for (wmOperator *op = (wmOperator *)(wm->runtime.operators.last); op; op = op->prev) {
		if (op->properties == ptr->data) {
			return op;
		}
	}

	return NULL;
}

StructRNA *rna_OperatorProperties_refine(PointerRNA *ptr) {
	wmOperator *op = rna_OperatorProperties_find_operator(ptr);

	if (op) {
		return op->type->srna;
	}
	return ptr->type;
}

/** \} */