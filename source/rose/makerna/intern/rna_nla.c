#include "DNA_action_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

ROSE_STATIC void rna_def_nlastrip(RoseRNA *rna) {
	StructRNA *nlastrip = RNA_def_struct(rna, "NlaStrip", NULL);
	RNA_def_struct_ui_text(nlastrip, "NLA Strip", "A container referencing an existing Action");
	RNA_def_struct_path_func(nlastrip, "rna_NlaStrip_path");

	do {
		/* name property */
		PropertyRNA *name = RNA_def_property(nlastrip, "name", PROP_STRING, PROP_NONE);
		RNA_def_property_ui_text(name, "Name", "");
		RNA_def_struct_name_property(nlastrip, name);
	} while(false);


}

void RNA_def_nla(RoseRNA *rna) {
	rna_def_nlastrip(rna);
}
