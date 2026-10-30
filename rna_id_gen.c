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

static PointerRNA UnkownType_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void UnkownType_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_UnkownType_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = UnkownType_rna_properties_get(iter);
	}
}

extern void UnkownType_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = UnkownType_rna_properties_get(iter);
	}
}

extern void UnkownType_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

static PointerRNA AnyType_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void AnyType_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_AnyType_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = AnyType_rna_properties_get(iter);
	}
}

extern void AnyType_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = AnyType_rna_properties_get(iter);
	}
}

extern void AnyType_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

static PointerRNA ID_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void ID_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_ID_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = ID_rna_properties_get(iter);
	}
}

extern void ID_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = ID_rna_properties_get(iter);
	}
}

extern void ID_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

extern void ID_name_set(PointerRNA *ptr, const char *value) {
	ID *data = (ID *)(ptr->data);
	LIB_strcpy(data->name, 64, value);
}

extern void ID_name_get(PointerRNA *ptr, char *value) {
	ID *data = (ID *)(ptr->data);
	strcpy(value, data->name);
}

extern int ID_name_length(PointerRNA *ptr) {
	ID *data = (ID *)(ptr->data);
	return strlen(data->name);
}

static PointerRNA PropertyGroupItem_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void PropertyGroupItem_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_PropertyGroupItem_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = PropertyGroupItem_rna_properties_get(iter);
	}
}

extern void PropertyGroupItem_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = PropertyGroupItem_rna_properties_get(iter);
	}
}

extern void PropertyGroupItem_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

static PointerRNA PropertyGroupItem_idp_array_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_iterator_array_get;
	return fn(iter);
}

extern void PropertyGroupItem_idp_array_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_PropertyGroupItem_idp_array_;

	PropCollectionBeginFunc fn = rna_IDPArray_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = PropertyGroupItem_idp_array_get(iter);
	}
}

extern void PropertyGroupItem_idp_array_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_iterator_array_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = PropertyGroupItem_idp_array_get(iter);
	}
}

extern void PropertyGroupItem_idp_array_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_iterator_array_end;
	fn(iter);
}

static PointerRNA PropertyGroup_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void PropertyGroup_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_PropertyGroup_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = PropertyGroup_rna_properties_get(iter);
	}
}

extern void PropertyGroup_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = PropertyGroup_rna_properties_get(iter);
	}
}

extern void PropertyGroup_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

/* Unknown Type */

CollectionPropertyRNA rna_UnkownType_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = NULL,
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
	.begin = UnkownType_rna_properties_begin,
	.next = UnkownType_rna_properties_next,
	.end = UnkownType_rna_properties_end,
	.get = UnkownType_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_UnkownType_rna_properties = (PropertyRNA *)&rna_UnkownType_rna_properties_.property;

StructRNA RNA_UnkownType = {
	.container = {
		.prev = (ContainerRNA *)&RNA_Property,
		.next = (ContainerRNA *)&RNA_AnyType,
		/* rna_properties */
		.properties = {
			.first = (PropertyRNA *)&rna_UnkownType_rna_properties_,
			.last = (PropertyRNA *)&rna_UnkownType_rna_properties_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "UnkownType",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Unknown Type",
	.description = "Stub RNA type used for pointers to unknown or internal data",
	.nameproperty = NULL,
	.iteratorproperty = (PropertyRNA *)&rna_UnkownType_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = NULL,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

/* Any Type */

CollectionPropertyRNA rna_AnyType_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = NULL,
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
	.begin = AnyType_rna_properties_begin,
	.next = AnyType_rna_properties_next,
	.end = AnyType_rna_properties_end,
	.get = AnyType_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_AnyType_rna_properties = (PropertyRNA *)&rna_AnyType_rna_properties_.property;

StructRNA RNA_AnyType = {
	.container = {
		.prev = (ContainerRNA *)&RNA_UnkownType,
		.next = (ContainerRNA *)&RNA_ID,
		/* rna_properties */
		.properties = {
			.first = (PropertyRNA *)&rna_AnyType_rna_properties_,
			.last = (PropertyRNA *)&rna_AnyType_rna_properties_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "AnyType",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Any Type",
	.description = "RNA type used for pointers to any possible data",
	.nameproperty = NULL,
	.iteratorproperty = (PropertyRNA *)&rna_AnyType_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = NULL,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

/* ID */

CollectionPropertyRNA rna_ID_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_ID_name_,
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
	.begin = ID_rna_properties_begin,
	.next = ID_rna_properties_next,
	.end = ID_rna_properties_end,
	.get = ID_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_ID_rna_properties = (PropertyRNA *)&rna_ID_rna_properties_.property;

StringPropertyRNA rna_ID_name_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_ID_rna_properties_,
		.next = NULL,
		.magic = -1,
		.identifier = "name",
		.flag = PROP_EDITABLE | PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Name",
		.description = "Unique data-block ID name (within a same type and library)",
		.arraydimension = 0,
		.arraylength = {66, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = ID_name_get,
	.length = ID_name_length,
	.set = ID_name_set,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 64,
	.defaultvalue = "",
};

PropertyRNA *rna_ID_name = (PropertyRNA *)&rna_ID_name_.property;

StructRNA RNA_ID = {
	.container = {
		.prev = (ContainerRNA *)&RNA_AnyType,
		.next = (ContainerRNA *)&RNA_PropertyGroupItem,
		/* rna_properties */
		/* name */
		.properties = {
			.first = (PropertyRNA *)&rna_ID_rna_properties_,
			.last = (PropertyRNA *)&rna_ID_name_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "ID",
	.flag = STRUCT_ID | STRUCT_ID_REFCOUNT | STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "ID",
	.description = "Base type for data-blocks, defining a unique name and garbage collection",
	.nameproperty = (PropertyRNA *)&rna_ID_name_,
	.iteratorproperty = (PropertyRNA *)&rna_ID_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = rna_ID_refine,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

/* ID Property */

CollectionPropertyRNA rna_PropertyGroupItem_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_string_,
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
	.begin = PropertyGroupItem_rna_properties_begin,
	.next = PropertyGroupItem_rna_properties_next,
	.end = PropertyGroupItem_rna_properties_end,
	.get = PropertyGroupItem_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_PropertyGroupItem_rna_properties = (PropertyRNA *)&rna_PropertyGroupItem_rna_properties_.property;

StringPropertyRNA rna_PropertyGroupItem_string_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_rna_properties_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_int_,
		.magic = -1,
		.identifier = "string",
		.flag = PROP_EDITABLE | PROP_NEVER_NULL | PROP_IDPROPERTY,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "string",
		.description = "",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.length = NULL,
	.set = NULL,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 0,
	.defaultvalue = "",
};

PropertyRNA *rna_PropertyGroupItem_string = (PropertyRNA *)&rna_PropertyGroupItem_string_.property;

IntPropertyRNA rna_PropertyGroupItem_int_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_string_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_int_array_,
		.magic = -1,
		.identifier = "int",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE | PROP_IDPROPERTY,
		.type = PROP_INT,
		.subtype = PROP_NONE,
		.name = "int",
		.description = "",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = NULL,
	.setarray = NULL,
	.range = NULL,
	.get_ex = NULL,
	.set_ex = NULL,
	.getarray_ex = NULL,
	.setarray_ex = NULL,
	.range_ex = NULL,
	.softmin = -10000,
	.softmax = 0,
	.hardmin = -2147483648,
	.hardmax = 10000,
	.step = 1,
	.get_default = NULL,
	.get_default_array = NULL,
	.defaultvalue = 0,
	.defaultarray = NULL
};

PropertyRNA *rna_PropertyGroupItem_int = (PropertyRNA *)&rna_PropertyGroupItem_int_.property;

static int rna_PropertyGroupItem_int_array_default[1] = {
	0
};

IntPropertyRNA rna_PropertyGroupItem_int_array_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_int_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_float_,
		.magic = -1,
		.identifier = "int_array",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE | PROP_IDPROPERTY,
		.type = PROP_INT,
		.subtype = PROP_NONE,
		.name = "int_array",
		.description = "",
		.arraydimension = 1,
		.arraylength = {1, 0, 0},
		.totarraylength = 1,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = NULL,
	.setarray = NULL,
	.range = NULL,
	.get_ex = NULL,
	.set_ex = NULL,
	.getarray_ex = NULL,
	.setarray_ex = NULL,
	.range_ex = NULL,
	.softmin = -10000,
	.softmax = 0,
	.hardmin = -2147483648,
	.hardmax = 10000,
	.step = 1,
	.get_default = NULL,
	.get_default_array = NULL,
	.defaultvalue = 0,
	.defaultarray = rna_PropertyGroupItem_int_array_default
};

PropertyRNA *rna_PropertyGroupItem_int_array = (PropertyRNA *)&rna_PropertyGroupItem_int_array_.property;

FloatPropertyRNA rna_PropertyGroupItem_float_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_int_array_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_float_array_,
		.magic = -1,
		.identifier = "float",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE | PROP_IDPROPERTY,
		.type = PROP_FLOAT,
		.subtype = PROP_NONE,
		.name = "float",
		.description = "",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = NULL,
	.setarray = NULL,
	.range = NULL,
	.get_ex = NULL,
	.set_ex = NULL,
	.getarray_ex = NULL,
	.setarray_ex = NULL,
	.range_ex = NULL,
	.softmin = -10000.0f,
	.softmax = 10000.0f,
	.hardmin = -FLT_MAX,
	.hardmax = FLT_MAX,
	.step = 10.0f,
	.precision = 3,
	.get_default = NULL,
	.get_default_array = NULL,
	.defaultvalue = 0.0f,
	.defaultarray = NULL
};

PropertyRNA *rna_PropertyGroupItem_float = (PropertyRNA *)&rna_PropertyGroupItem_float_.property;

static float rna_PropertyGroupItem_float_array_default[1] = {
	0.0f
};

FloatPropertyRNA rna_PropertyGroupItem_float_array_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_float_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_double_,
		.magic = -1,
		.identifier = "float_array",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE | PROP_IDPROPERTY,
		.type = PROP_FLOAT,
		.subtype = PROP_NONE,
		.name = "float_array",
		.description = "",
		.arraydimension = 1,
		.arraylength = {1, 0, 0},
		.totarraylength = 1,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = NULL,
	.setarray = NULL,
	.range = NULL,
	.get_ex = NULL,
	.set_ex = NULL,
	.getarray_ex = NULL,
	.setarray_ex = NULL,
	.range_ex = NULL,
	.softmin = -10000.0f,
	.softmax = 10000.0f,
	.hardmin = -FLT_MAX,
	.hardmax = FLT_MAX,
	.step = 10.0f,
	.precision = 3,
	.get_default = NULL,
	.get_default_array = NULL,
	.defaultvalue = 0.0f,
	.defaultarray = rna_PropertyGroupItem_float_array_default
};

PropertyRNA *rna_PropertyGroupItem_float_array = (PropertyRNA *)&rna_PropertyGroupItem_float_array_.property;

FloatPropertyRNA rna_PropertyGroupItem_double_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_float_array_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_double_array_,
		.magic = -1,
		.identifier = "double",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE | PROP_IDPROPERTY,
		.type = PROP_FLOAT,
		.subtype = PROP_NONE,
		.name = "double",
		.description = "",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = NULL,
	.setarray = NULL,
	.range = NULL,
	.get_ex = NULL,
	.set_ex = NULL,
	.getarray_ex = NULL,
	.setarray_ex = NULL,
	.range_ex = NULL,
	.softmin = -10000.0f,
	.softmax = 10000.0f,
	.hardmin = -FLT_MAX,
	.hardmax = FLT_MAX,
	.step = 10.0f,
	.precision = 3,
	.get_default = NULL,
	.get_default_array = NULL,
	.defaultvalue = 0.0f,
	.defaultarray = NULL
};

PropertyRNA *rna_PropertyGroupItem_double = (PropertyRNA *)&rna_PropertyGroupItem_double_.property;

static float rna_PropertyGroupItem_double_array_default[1] = {
	0.0f
};

FloatPropertyRNA rna_PropertyGroupItem_double_array_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_double_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_bool_,
		.magic = -1,
		.identifier = "double_array",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE | PROP_IDPROPERTY,
		.type = PROP_FLOAT,
		.subtype = PROP_NONE,
		.name = "double_array",
		.description = "",
		.arraydimension = 1,
		.arraylength = {1, 0, 0},
		.totarraylength = 1,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = NULL,
	.setarray = NULL,
	.range = NULL,
	.get_ex = NULL,
	.set_ex = NULL,
	.getarray_ex = NULL,
	.setarray_ex = NULL,
	.range_ex = NULL,
	.softmin = -10000.0f,
	.softmax = 10000.0f,
	.hardmin = -FLT_MAX,
	.hardmax = FLT_MAX,
	.step = 10.0f,
	.precision = 3,
	.get_default = NULL,
	.get_default_array = NULL,
	.defaultvalue = 0.0f,
	.defaultarray = rna_PropertyGroupItem_double_array_default
};

PropertyRNA *rna_PropertyGroupItem_double_array = (PropertyRNA *)&rna_PropertyGroupItem_double_array_.property;

BoolPropertyRNA rna_PropertyGroupItem_bool_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_double_array_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_bool_array_,
		.magic = -1,
		.identifier = "bool",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE | PROP_IDPROPERTY,
		.type = PROP_BOOLEAN,
		.subtype = PROP_NONE,
		.name = "bool",
		.description = "",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = NULL,
	.setarray = NULL,
	.get_ex = NULL,
	.set_ex = NULL,
	.getarray_ex = NULL,
	.setarray_ex = NULL,
	.get_default = NULL,
	.get_default_array = NULL,
	.defaultvalue = 0,
	.defaultarray = NULL
};

PropertyRNA *rna_PropertyGroupItem_bool = (PropertyRNA *)&rna_PropertyGroupItem_bool_.property;

static bool rna_PropertyGroupItem_bool_array_default[1] = {
	0
};

BoolPropertyRNA rna_PropertyGroupItem_bool_array_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_bool_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_group_,
		.magic = -1,
		.identifier = "bool_array",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE | PROP_IDPROPERTY,
		.type = PROP_BOOLEAN,
		.subtype = PROP_NONE,
		.name = "bool_array",
		.description = "",
		.arraydimension = 1,
		.arraylength = {1, 0, 0},
		.totarraylength = 1,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = NULL,
	.setarray = NULL,
	.get_ex = NULL,
	.set_ex = NULL,
	.getarray_ex = NULL,
	.setarray_ex = NULL,
	.get_default = NULL,
	.get_default_array = NULL,
	.defaultvalue = 0,
	.defaultarray = rna_PropertyGroupItem_bool_array_default
};

PropertyRNA *rna_PropertyGroupItem_bool_array = (PropertyRNA *)&rna_PropertyGroupItem_bool_array_.property;

PointerPropertyRNA rna_PropertyGroupItem_group_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_bool_array_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_collection_,
		.magic = -1,
		.identifier = "group",
		.flag = PROP_IDPROPERTY,
		.type = PROP_POINTER,
		.subtype = PROP_NONE,
		.name = "group",
		.description = "",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.type = NULL,
	.poll = NULL,
	.srna = &RNA_PropertyGroup,
};

PropertyRNA *rna_PropertyGroupItem_group = (PropertyRNA *)&rna_PropertyGroupItem_group_.property;

CollectionPropertyRNA rna_PropertyGroupItem_collection_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_group_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_idp_array_,
		.magic = -1,
		.identifier = "collection",
		.flag = PROP_IDPROPERTY,
		.type = PROP_COLLECTION,
		.subtype = PROP_NONE,
		.name = "collection",
		.description = "",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.begin = NULL,
	.next = NULL,
	.end = NULL,
	.get = NULL,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = NULL,
	.element = &RNA_PropertyGroup,
};

PropertyRNA *rna_PropertyGroupItem_collection = (PropertyRNA *)&rna_PropertyGroupItem_collection_.property;

CollectionPropertyRNA rna_PropertyGroupItem_idp_array_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_collection_,
		.next = (PropertyRNA *)&rna_PropertyGroupItem_id_,
		.magic = -1,
		.identifier = "idp_array",
		.flag = PROP_IDPROPERTY,
		.type = PROP_COLLECTION,
		.subtype = PROP_NONE,
		.name = "idp_array",
		.description = "",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.begin = PropertyGroupItem_idp_array_begin,
	.next = PropertyGroupItem_idp_array_next,
	.end = PropertyGroupItem_idp_array_end,
	.get = PropertyGroupItem_idp_array_get,
	.length = rna_IDPArray_length,
	.lookupint = NULL,
	.lookupstring = NULL,
	.element = &RNA_PropertyGroup,
};

PropertyRNA *rna_PropertyGroupItem_idp_array = (PropertyRNA *)&rna_PropertyGroupItem_idp_array_.property;

PointerPropertyRNA rna_PropertyGroupItem_id_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroupItem_idp_array_,
		.next = NULL,
		.magic = -1,
		.identifier = "id",
		.flag = PROP_EDITABLE | PROP_PTR_NO_OWNERSHIP | PROP_IDPROPERTY,
		.type = PROP_POINTER,
		.subtype = PROP_NONE,
		.name = "id",
		.description = "",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.type = NULL,
	.poll = NULL,
	.srna = &RNA_ID,
};

PropertyRNA *rna_PropertyGroupItem_id = (PropertyRNA *)&rna_PropertyGroupItem_id_.property;

StructRNA RNA_PropertyGroupItem = {
	.container = {
		.prev = (ContainerRNA *)&RNA_ID,
		.next = (ContainerRNA *)&RNA_PropertyGroup,
		/* rna_properties */
		/* string */
		/* int */
		/* int_array */
		/* float */
		/* float_array */
		/* double */
		/* double_array */
		/* bool */
		/* bool_array */
		/* group */
		/* collection */
		/* idp_array */
		/* id */
		.properties = {
			.first = (PropertyRNA *)&rna_PropertyGroupItem_rna_properties_,
			.last = (PropertyRNA *)&rna_PropertyGroupItem_id_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "PropertyGroupItem",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "ID Property",
	.description = "Property that stores arbitrary, user defined properties",
	.nameproperty = NULL,
	.iteratorproperty = (PropertyRNA *)&rna_PropertyGroupItem_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = NULL,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

/* ID Property Group */

CollectionPropertyRNA rna_PropertyGroup_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_PropertyGroup_name_,
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
	.begin = PropertyGroup_rna_properties_begin,
	.next = PropertyGroup_rna_properties_next,
	.end = PropertyGroup_rna_properties_end,
	.get = PropertyGroup_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_PropertyGroup_rna_properties = (PropertyRNA *)&rna_PropertyGroup_rna_properties_.property;

StringPropertyRNA rna_PropertyGroup_name_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PropertyGroup_rna_properties_,
		.next = NULL,
		.magic = -1,
		.identifier = "name",
		.flag = PROP_EDITABLE | PROP_NEVER_NULL | PROP_IDPROPERTY,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Name",
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
	.get = NULL,
	.length = NULL,
	.set = NULL,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 0,
	.defaultvalue = "",
};

PropertyRNA *rna_PropertyGroup_name = (PropertyRNA *)&rna_PropertyGroup_name_.property;

StructRNA RNA_PropertyGroup = {
	.container = {
		.prev = (ContainerRNA *)&RNA_PropertyGroupItem,
		.next = (ContainerRNA *)&RNA_Object,
		/* rna_properties */
		/* name */
		.properties = {
			.first = (PropertyRNA *)&rna_PropertyGroup_rna_properties_,
			.last = (PropertyRNA *)&rna_PropertyGroup_name_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "PropertyGroup",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "ID Property Group",
	.description = "Group of ID properties",
	.nameproperty = (PropertyRNA *)&rna_PropertyGroup_name_,
	.iteratorproperty = (PropertyRNA *)&rna_PropertyGroup_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = rna_PropertyGroup_refine,
	.path = NULL,
	.idproperties = rna_PropertyGroup_idprops,
	.system_idproperties = rna_PropertyGroup_idprops,
};

