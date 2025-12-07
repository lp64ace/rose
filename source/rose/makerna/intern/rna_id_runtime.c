#include "DNA_ID.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_prototypes.h"

#include "rna_internal_types.h"
#include "rna_internal.h"

StructRNA *RNA_id_code_to_rna_type(int type) {
	switch (type) {
		case ID_OB: return &RNA_Object;
	}

	return &RNA_ID;
}

/* -------------------------------------------------------------------- */
/** \name Exposed Functions
 * \{ */

StructRNA *rna_ID_refine(PointerRNA *ptr) {
	ID *id = (ID *)ptr->data;

	return RNA_id_code_to_rna_type(GS(id->name));
}

/** \} */
