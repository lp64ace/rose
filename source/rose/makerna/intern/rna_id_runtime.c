#include "DNA_ID.h"

#include "KER_idprop.h"

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

/* -------------------------------------------------------------------- */
/** \name IDProperty RNA Definition Functions
 * \{ */

void rna_IDPArray_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	IDProperty *prop = (IDProperty *)ptr->data;
	rna_iterator_array_begin(iter, ptr, IDP_Array(prop), sizeof(IDProperty), prop->length, 0, NULL);
}

void rna_IDPArray_length(PointerRNA *ptr) {
	IDProperty *prop = (IDProperty *)ptr->data;
	return prop->length;
}

IDProperty **rna_PropertyGroup_idprops(PointerRNA *ptr) {
	return (IDProperty **)&ptr->data;
}

StructRNA *rna_PropertyGroup_refine(PointerRNA *ptr) {
	return ptr->type;
}

/** \} */
