#include "DNA_ID.h"

#include "RNA_define.h"

#include "rna_internal_types.h"
#include "rna_internal.h"

ROSE_STATIC void rna_def_id(RoseRNA *rna) {
	do {
		StructRNA *id = RNA_def_struct(rna, "ID", NULL);
		RNA_def_struct_ui_text(id, "ID", "Base type for data-blocks, defining a unique name and garbage collection");
		RNA_def_struct_flag(id, STRUCT_ID | STRUCT_ID_REFCOUNT);

		do {
			PropertyRNA *name = RNA_def_property(id, "name", PROP_STRING, PROP_NONE);
			RNA_def_property_ui_text(name, "Name", "Unique data-block ID name (within a same type and library)");
			RNA_def_property_string_maxlength(name, ARRAY_SIZE(((ID *)NULL)->name) - 2);
			RNA_def_struct_name_property(id, name);
		} while (false);
	} while (false);
}

void RNA_def_ID(RoseRNA *rna) {
	do {
		StructRNA *unkown = RNA_def_struct(rna, "UnkownType", NULL);
		RNA_def_struct_ui_text(unkown, "Unknown Type", "Stub RNA type used for pointers to unknown or internal data");
	} while (false);

	do {
		StructRNA *any = RNA_def_struct(rna, "AnyType", NULL);
		RNA_def_struct_ui_text(any, "Any Type", "RNA type used for pointers to any possible data");
	} while (false);

	rna_def_id(rna);
}
