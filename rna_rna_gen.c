/**
 * Automatically generated struct definitions for the Data API.
 * Do not edit manually, changes will be overwritten.
 */

#include <float.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stddef.h>

#include "MEM_guardedalloc.h"

#include "DNA_include_all.h"

#include "LIB_listbase.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_lib_id.h"
#include "KER_main.h"

#include "RNA_access.h"
#include "RNA_prototypes.h"

#include "rna_internal_types.h"
#include "rna_internal.h"

static PointerRNA Struct_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void Struct_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_Struct_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = Struct_rna_properties_get(iter);
	}
}

extern void Struct_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = Struct_rna_properties_get(iter);
	}
}

extern void Struct_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

extern void Struct_name_get(PointerRNA *ptr, char *value) {
	PropStringGetFunc fn = rna_Struct_name_get;
	fn(ptr, value);
}

extern int Struct_name_length(PointerRNA *ptr) {
	PropStringLengthFunc fn = rna_Struct_name_length;
	return fn(ptr);
}

extern void Struct_identifier_get(PointerRNA *ptr, char *value) {
	PropStringGetFunc fn = rna_Struct_identifier_get;
	fn(ptr, value);
}

extern int Struct_identifier_length(PointerRNA *ptr) {
	PropStringLengthFunc fn = rna_Struct_identifier_length;
	return fn(ptr);
}

extern void Struct_description_get(PointerRNA *ptr, char *value) {
	PropStringGetFunc fn = rna_Struct_description_get;
	fn(ptr, value);
}

extern int Struct_description_length(PointerRNA *ptr) {
	PropStringLengthFunc fn = rna_Struct_description_length;
	return fn(ptr);
}

static PointerRNA Struct_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_Struct_properties_get;
	return fn(iter);
}

extern void Struct_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_Struct_properties_;

	PropCollectionBeginFunc fn = rna_Struct_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = Struct_properties_get(iter);
	}
}

extern void Struct_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_Struct_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = Struct_properties_get(iter);
	}
}

extern void Struct_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_iterator_listbase_end;
	fn(iter);
}

static PointerRNA Property_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void Property_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_Property_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = Property_rna_properties_get(iter);
	}
}

extern void Property_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = Property_rna_properties_get(iter);
	}
}

extern void Property_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

extern void Property_name_get(PointerRNA *ptr, char *value) {
	PropStringGetFunc fn = rna_Property_name_get;
	fn(ptr, value);
}

extern int Property_name_length(PointerRNA *ptr) {
	PropStringLengthFunc fn = rna_Property_name_length;
	return fn(ptr);
}

extern void Property_identifier_get(PointerRNA *ptr, char *value) {
	PropStringGetFunc fn = rna_Property_identifier_get;
	fn(ptr, value);
}

extern int Property_identifier_length(PointerRNA *ptr) {
	PropStringLengthFunc fn = rna_Property_identifier_length;
	return fn(ptr);
}

extern void Property_description_get(PointerRNA *ptr, char *value) {
	PropStringGetFunc fn = rna_Property_description_get;
	fn(ptr, value);
}

extern int Property_description_length(PointerRNA *ptr) {
	PropStringLengthFunc fn = rna_Property_description_length;
	return fn(ptr);
}

/* Struct Definition */

CollectionPropertyRNA rna_Struct_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_Struct_name_,
		.magic = -1,
		.identifier = "rna_properties",
		.flag = 0,
		.type = PROP_COLLECTION,
		.subtype = PROP_NONE,
		.name = "RNA Properties",
		.description = "RNA property collection",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.begin = Struct_rna_properties_begin,
	.next = Struct_rna_properties_next,
	.end = Struct_rna_properties_end,
	.get = Struct_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_Struct_rna_properties = (PropertyRNA *)&rna_Struct_rna_properties_.property;

StringPropertyRNA rna_Struct_name_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Struct_rna_properties_,
		.next = (PropertyRNA *)&rna_Struct_identifier_,
		.magic = -1,
		.identifier = "name",
		.flag = PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Name",
		.description = "Human readable name",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = Struct_name_get,
	.length = Struct_name_length,
	.set = NULL,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 0,
	.defaultvalue = "",
};

PropertyRNA *rna_Struct_name = (PropertyRNA *)&rna_Struct_name_.property;

StringPropertyRNA rna_Struct_identifier_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Struct_name_,
		.next = (PropertyRNA *)&rna_Struct_description_,
		.magic = -1,
		.identifier = "identifier",
		.flag = PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Identifier",
		.description = "Unique name used in the code and scripting",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = Struct_identifier_get,
	.length = Struct_identifier_length,
	.set = NULL,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 0,
	.defaultvalue = "",
};

PropertyRNA *rna_Struct_identifier = (PropertyRNA *)&rna_Struct_identifier_.property;

StringPropertyRNA rna_Struct_description_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Struct_identifier_,
		.next = (PropertyRNA *)&rna_Struct_properties_,
		.magic = -1,
		.identifier = "description",
		.flag = PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Description",
		.description = "Description of the Struct\'s purpose",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = Struct_description_get,
	.length = Struct_description_length,
	.set = NULL,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 0,
	.defaultvalue = "",
};

PropertyRNA *rna_Struct_description = (PropertyRNA *)&rna_Struct_description_.property;

CollectionPropertyRNA rna_Struct_properties_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Struct_description_,
		.next = NULL,
		.magic = -1,
		.identifier = "properties",
		.flag = 0,
		.type = PROP_COLLECTION,
		.subtype = PROP_NONE,
		.name = "Properties",
		.description = "Properties in the struct",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.begin = Struct_properties_begin,
	.next = Struct_properties_next,
	.end = Struct_properties_end,
	.get = Struct_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = NULL,
	.element = &RNA_Property,
};

PropertyRNA *rna_Struct_properties = (PropertyRNA *)&rna_Struct_properties_.property;

StructRNA RNA_Struct = {
	.container = {
		.prev = NULL,
		.next = (ContainerRNA *)&RNA_Property,
		/* rna_properties */
		/* name */
		/* identifier */
		/* description */
		/* properties */
		.properties = {
			.first = (PropertyRNA *)&rna_Struct_rna_properties_,
			.last = (PropertyRNA *)&rna_Struct_properties_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "Struct",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Struct Definition",
	.description = "RNA structure definition",
	.nameproperty = (PropertyRNA *)&rna_Struct_name_,
	.iteratorproperty = (PropertyRNA *)&rna_Struct_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = NULL,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

/* Property Definition */

CollectionPropertyRNA rna_Property_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_Property_name_,
		.magic = -1,
		.identifier = "rna_properties",
		.flag = 0,
		.type = PROP_COLLECTION,
		.subtype = PROP_NONE,
		.name = "RNA Properties",
		.description = "RNA property collection",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.begin = Property_rna_properties_begin,
	.next = Property_rna_properties_next,
	.end = Property_rna_properties_end,
	.get = Property_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_Property_rna_properties = (PropertyRNA *)&rna_Property_rna_properties_.property;

StringPropertyRNA rna_Property_name_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Property_rna_properties_,
		.next = (PropertyRNA *)&rna_Property_identifier_,
		.magic = -1,
		.identifier = "name",
		.flag = PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Name",
		.description = "Human readable name",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = Property_name_get,
	.length = Property_name_length,
	.set = NULL,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 0,
	.defaultvalue = "",
};

PropertyRNA *rna_Property_name = (PropertyRNA *)&rna_Property_name_.property;

StringPropertyRNA rna_Property_identifier_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Property_name_,
		.next = (PropertyRNA *)&rna_Property_description_,
		.magic = -1,
		.identifier = "identifier",
		.flag = PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Identifier",
		.description = "Unique name used in the code and scripting",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = Property_identifier_get,
	.length = Property_identifier_length,
	.set = NULL,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 0,
	.defaultvalue = "",
};

PropertyRNA *rna_Property_identifier = (PropertyRNA *)&rna_Property_identifier_.property;

StringPropertyRNA rna_Property_description_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Property_identifier_,
		.next = NULL,
		.magic = -1,
		.identifier = "description",
		.flag = PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Description",
		.description = "Description of the property for tooltips",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = Property_description_get,
	.length = Property_description_length,
	.set = NULL,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 0,
	.defaultvalue = "",
};

PropertyRNA *rna_Property_description = (PropertyRNA *)&rna_Property_description_.property;

StructRNA RNA_Property = {
	.container = {
		.prev = (ContainerRNA *)&RNA_Struct,
		.next = (ContainerRNA *)&RNA_UnkownType,
		/* rna_properties */
		/* name */
		/* identifier */
		/* description */
		.properties = {
			.first = (PropertyRNA *)&rna_Property_rna_properties_,
			.last = (PropertyRNA *)&rna_Property_description_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "Property",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Property Definition",
	.description = "RNA property definition",
	.nameproperty = (PropertyRNA *)&rna_Property_name_,
	.iteratorproperty = (PropertyRNA *)&rna_Property_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = NULL,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

RoseRNA ROSE_RNA = {
	.srnabase = {
		/* Struct */
		/* Property */
		/* UnkownType */
		/* AnyType */
		/* ID */
		/* PropertyGroupItem */
		/* PropertyGroup */
		/* Object */
		/* ObjectBase */
		/* Pose */
		/* PoseBone */
		/* FileSelectParams */
		/* OperatorProperties */
		/* OperatorFileListElement */
		.first = &RNA_Struct,
		.last = &RNA_OperatorFileListElement,
	},
	.srnahash = NULL,
	.totsrna = 0,
};
