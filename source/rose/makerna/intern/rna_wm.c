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

ROSE_INLINE void rna_def_operator_filelist_element(RoseRNA *rna) {
	do {
		StructRNA *operatorfilelistelement = RNA_def_struct(rna, "OperatorFileListElement", "PropertyGroup");
		RNA_def_struct_ui_text(operatorfilelistelement, "Operator File List Element", "");

		do {
			PropertyRNA *name = RNA_def_property(operatorfilelistelement, "name", PROP_STRING, PROP_FILENAME);
			RNA_def_property_flag(name, PROP_IDPROPERTY);
			RNA_def_property_ui_text(name, "Name", "Name of a file or directory within a file list");
		} while (false);
	} while (false);
}

void RNA_def_wm(RoseRNA *rna) {
	rna_def_operator(rna);
	rna_def_operator_filelist_element(rna);
}
