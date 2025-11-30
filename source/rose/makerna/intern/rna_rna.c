#include "DNA_object_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

ROSE_INLINE void rna_def_struct(RoseRNA *rna) {
	do {
		StructRNA *structrna = RNA_def_struct(rna, "Struct", NULL);

		RNA_def_struct_ui_text(structrna, "Struct Definition", "RNA structure definition");

		do {
			PropertyRNA *name = RNA_def_property(structrna, "name", PROP_STRING, PROP_NONE);
			RNA_def_struct_name_property(structrna, name);
			RNA_def_property_clear_flag(name, PROP_EDITABLE);
			RNA_def_property_ui_text(name, "Name", "Human readable name");
		} while (false);

		do {
			PropertyRNA *identifier = RNA_def_property(structrna, "identifier", PROP_STRING, PROP_NONE);
			RNA_def_property_clear_flag(identifier, PROP_EDITABLE);
			RNA_def_property_ui_text(identifier, "Identifier", "Unique name used in the code and scripting");
		} while (false);

		do {
			PropertyRNA *description = RNA_def_property(structrna, "description", PROP_STRING, PROP_NONE);
			RNA_def_property_clear_flag(description, PROP_EDITABLE);
			RNA_def_property_ui_text(description, "Description", "Description of the Struct's purpose");
		} while (false);

		do {
			PropertyRNA *properties = RNA_def_property(structrna, "properties", PROP_COLLECTION, PROP_NONE);
			RNA_def_property_clear_flag(properties, PROP_EDITABLE);
			RNA_def_property_struct_type(properties, "Property");
			RNA_def_property_collection_funcs(properties, "rna_Struct_properties_begin", "rna_Struct_properties_next", "rna_iterator_listbase_end", "rna_Struct_properties_get", NULL, NULL, NULL);
			RNA_def_property_ui_text(properties, "Properties", "Properties in the struct");
		} while (false);
	} while (false);
}

ROSE_INLINE void rna_def_property(RoseRNA *rna) {
	do {
		StructRNA *propertyrna = RNA_def_struct(rna, "Property", NULL);

		RNA_def_struct_ui_text(propertyrna, "Property Definition", "RNA property definition");

		do {
			PropertyRNA *name = RNA_def_property(propertyrna, "name", PROP_STRING, PROP_NONE);
			RNA_def_struct_name_property(propertyrna, name);
			RNA_def_property_clear_flag(name, PROP_EDITABLE);
			RNA_def_property_string_funcs(name, "rna_Property_name_get", "rna_Property_name_length", NULL);
			RNA_def_property_ui_text(name, "Name", "Human readable name");
		} while (false);

		do {
			PropertyRNA *identifier = RNA_def_property(propertyrna, "identifier", PROP_STRING, PROP_NONE);
			RNA_def_property_clear_flag(identifier, PROP_EDITABLE);
			RNA_def_property_ui_text(identifier, "Identifier", "Unique name used in the code and scripting");
		} while (false);

		do {
			PropertyRNA *description = RNA_def_property(propertyrna, "description", PROP_STRING, PROP_NONE);
			RNA_def_property_clear_flag(description, PROP_EDITABLE);
			RNA_def_property_ui_text(description, "Description", "Description of the property for tooltips");
		} while (false);
	} while (false);
}

void RNA_def_rna(RoseRNA *rna) {
	rna_def_struct(rna);
	rna_def_property(rna);
}
