#include "DNA_ID.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "rna_internal_types.h"
#include "rna_internal.h"

ROSE_STATIC void rna_def_ID(RoseRNA *rna) {
	do {
		StructRNA *id = RNA_def_struct(rna, "ID", NULL);
		RNA_def_struct_ui_text(id, "ID", "Base type for data-blocks, defining a unique name and garbage collection");
		/**
		 * This is needed so that when we create a pointer for an ID, 
		 * we can get the exact type from it.
		 * 
		 * e.g. #ID -> (#Object | #Mesh | ...)
		 */
		RNA_def_struct_refine_func(id, "rna_ID_refine");
		RNA_def_struct_flag(id, STRUCT_ID | STRUCT_ID_REFCOUNT);

		do {
			PropertyRNA *name = RNA_def_property(id, "name", PROP_STRING, PROP_NONE);
			RNA_def_property_ui_text(name, "Name", "Unique data-block ID name (within a same type and library)");
			RNA_def_property_string_maxlength(name, ARRAY_SIZE(((ID *)NULL)->name) - 2);
			RNA_def_struct_name_property(id, name);
		} while (false);
	} while (false);
}

ROSE_STATIC void rna_def_ID_properties(RoseRNA *rna) {
	do {
		StructRNA *group = RNA_def_struct(rna, "PropertyGroupItem", NULL);
		RNA_def_struct_sdna(group, "IDProperty");
		RNA_def_struct_ui_text(group, "ID Property", "Property that stores arbitrary, user defined properties");

		/* IDP_STRING */
		do {
			PropertyRNA *property = RNA_def_property(group, "string", PROP_STRING, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
		} while (false);

		/* IDP_INT */
		do {
			PropertyRNA *property = RNA_def_property(group, "int", PROP_INT, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
		} while (false);

		do {
			PropertyRNA *property = RNA_def_property(group, "int_array", PROP_INT, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
			RNA_def_property_array(property, 1);
		} while (false);

		/* IDP_FLOAT */
		do {
			PropertyRNA *property = RNA_def_property(group, "float", PROP_FLOAT, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
		} while (false);

		do {
			PropertyRNA *property = RNA_def_property(group, "float_array", PROP_FLOAT, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
			RNA_def_property_array(property, 1);
		} while (false);

		/* IDP_DOUBLE */
		do {
			PropertyRNA *property = RNA_def_property(group, "double", PROP_FLOAT, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
		} while (false);

		do {
			PropertyRNA *property = RNA_def_property(group, "double_array", PROP_FLOAT, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
			RNA_def_property_array(property, 1);
		} while (false);

		/* IDP_BOOLEAN */
		do {
			PropertyRNA *property = RNA_def_property(group, "bool", PROP_BOOLEAN, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
		} while (false);

		do {
			PropertyRNA *property = RNA_def_property(group, "bool_array", PROP_BOOLEAN, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
			RNA_def_property_array(property, 1);
		} while (false);

		/* IDP_GROUP */
		do {
			PropertyRNA *property = RNA_def_property(group, "group", PROP_POINTER, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
			RNA_def_property_clear_flag(property, PROP_EDITABLE);
			RNA_def_property_struct_type(property, "PropertyGroup");
		} while (false);

		do {
			PropertyRNA *property = RNA_def_property(group, "collection", PROP_COLLECTION, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
			RNA_def_property_struct_type(property, "PropertyGroup");
		} while (false);

		do {
			PropertyRNA *property = RNA_def_property(group, "idp_array", PROP_COLLECTION, PROP_NONE);
			RNA_def_property_struct_type(property, "PropertyGroup");
			RNA_def_property_collection_funcs(property, "rna_IDPArray_begin", "rna_iterator_array_next", "rna_iterator_array_end", "rna_iterator_array_get", "rna_IDPArray_length", NULL, NULL);
			RNA_def_property_flag(property, PROP_IDPROPERTY);
		} while (false);

		/* IDP_ID */
		do {
			PropertyRNA *property = RNA_def_property(group, "id", PROP_POINTER, PROP_NONE);
			RNA_def_property_flag(property, PROP_IDPROPERTY | PROP_EDITABLE);
			RNA_def_property_struct_type(property, "ID");
		} while (false);
	} while (false);

	do {
		StructRNA *group = RNA_def_struct(rna, "PropertyGroup", NULL);
		RNA_def_struct_sdna(group, "IDPropertyGroup");
		RNA_def_struct_ui_text(group, "ID Property Group", "Group of ID properties");
		/* For property groups, both 'user-defined' and system-defined properties are the same.
		 * The user-defined access is kept to allow 'dict-type' subscripting in python. */
		RNA_def_struct_idprops_func(group, "rna_PropertyGroup_idprops");
		RNA_def_struct_system_idprops_func(group, "rna_PropertyGroup_idprops");
		RNA_def_struct_refine_func(group, "rna_PropertyGroup_refine");

		do {
			/* important so python types can have their name used in list views
			 * however this isn't perfect because it overrides how python would set the name
			 * when we only really want this so RNA_def_struct_name_property() is set to something useful */
			PropertyRNA *name = RNA_def_property(group, "name", PROP_STRING, PROP_NONE);
			RNA_def_property_flag(name, PROP_IDPROPERTY);
			// RNA_def_property_clear_flag(prop, PROP_EDITABLE);
			RNA_def_property_ui_text(name, "Name", "Unique name used in the code and scripting");
			RNA_def_struct_name_property(group, name);
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

	rna_def_ID(rna);
	rna_def_ID_properties(rna);
}
