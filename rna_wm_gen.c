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

static PointerRNA OperatorProperties_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void OperatorProperties_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_OperatorProperties_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = OperatorProperties_rna_properties_get(iter);
	}
}

extern void OperatorProperties_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = OperatorProperties_rna_properties_get(iter);
	}
}

extern void OperatorProperties_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

/* Operator Properties */

CollectionPropertyRNA rna_OperatorProperties_rna_properties_ = {
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
	.begin = OperatorProperties_rna_properties_begin,
	.next = OperatorProperties_rna_properties_next,
	.end = OperatorProperties_rna_properties_end,
	.get = OperatorProperties_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_OperatorProperties_rna_properties = (PropertyRNA *)&rna_OperatorProperties_rna_properties_.property;

StructRNA RNA_OperatorProperties = {
	.container = {
		.prev = (ContainerRNA *)&RNA_FileSelectParams,
		.next = (ContainerRNA *)&RNA_OperatorFileListElement,
		/* rna_properties */
		.properties = {
			.first = (PropertyRNA *)&rna_OperatorProperties_rna_properties_,
			.last = (PropertyRNA *)&rna_OperatorProperties_rna_properties_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "OperatorProperties",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Operator Properties",
	.description = "Input properties of an operator",
	.nameproperty = NULL,
	.iteratorproperty = (PropertyRNA *)&rna_OperatorProperties_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = rna_OperatorProperties_refine,
	.path = NULL,
	.idproperties = rna_OperatorProperties_idprops,
	.system_idproperties = rna_OperatorProperties_idprops,
};

/* Operator File List Element */

StringPropertyRNA rna_OperatorFileListElement_name_ = {
	.property = {
		.prev = NULL,
		.next = NULL,
		.magic = -1,
		.identifier = "name",
		.flag = PROP_EDITABLE | PROP_NEVER_NULL | PROP_IDPROPERTY,
		.type = PROP_STRING,
		.subtype = PROP_FILENAME,
		.name = "Name",
		.description = "Name of a file or directory within a file list",
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

PropertyRNA *rna_OperatorFileListElement_name = (PropertyRNA *)&rna_OperatorFileListElement_name_.property;

StructRNA RNA_OperatorFileListElement = {
	.container = {
		.prev = (ContainerRNA *)&RNA_OperatorProperties,
		.next = NULL,
		/* name */
		.properties = {
			.first = (PropertyRNA *)&rna_OperatorFileListElement_name_,
			.last = (PropertyRNA *)&rna_OperatorFileListElement_name_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "OperatorFileListElement",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Operator File List Element",
	.description = "",
	.nameproperty = (PropertyRNA *)&rna_PropertyGroup_name_,
	.iteratorproperty = (PropertyRNA *)&rna_PropertyGroup_rna_properties_,
	.base = &RNA_PropertyGroup,
	.nested = NULL,
	.refine = rna_PropertyGroup_refine,
	.path = NULL,
	.idproperties = rna_PropertyGroup_idprops,
	.system_idproperties = rna_PropertyGroup_idprops,
};

