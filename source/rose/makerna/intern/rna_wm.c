#include "DNA_object_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

ROSE_INLINE void rna_def_operator(RoseRNA *rna) {
	do {
		StructRNA *operatorproperties = RNA_def_struct(rna, "OperatorProperties", NULL);

		RNA_def_struct_ui_text(operatorproperties, "Operator Properties", "Input properties of an operator");
		RNA_def_struct_refine_func(operatorproperties, "rna_OperatorProperties_refine");
		RNA_def_struct_idprops_func(operatorproperties, "rna_OperatorProperties_idprops");
		RNA_def_struct_system_idprops_func(operatorproperties, "rna_OperatorProperties_idprops");
	} while (false);
}

void RNA_def_wm(RoseRNA *rna) {
	rna_def_operator(rna);
}
