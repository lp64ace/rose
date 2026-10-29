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

static PointerRNA FileSelectParams_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void FileSelectParams_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_FileSelectParams_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = FileSelectParams_rna_properties_get(iter);
	}
}

extern void FileSelectParams_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = FileSelectParams_rna_properties_get(iter);
	}
}

extern void FileSelectParams_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

extern void FileSelectParams_title_get(PointerRNA *ptr, char *value) {
	FileSelectParams *data = (FileSelectParams *)(ptr->data);
	strcpy(value, data->title);
}

extern int FileSelectParams_title_length(PointerRNA *ptr) {
	FileSelectParams *data = (FileSelectParams *)(ptr->data);
	return strlen(data->title);
}

extern void FileSelectParams_directory_set(PointerRNA *ptr, const char *value) {
	FileSelectParams *data = (FileSelectParams *)(ptr->data);
	LIB_strcpy(data->dir, 1024, value);
}

extern void FileSelectParams_directory_get(PointerRNA *ptr, char *value) {
	FileSelectParams *data = (FileSelectParams *)(ptr->data);
	strcpy(value, data->dir);
}

extern int FileSelectParams_directory_length(PointerRNA *ptr) {
	FileSelectParams *data = (FileSelectParams *)(ptr->data);
	return strlen(data->dir);
}

extern void FileSelectParams_filename_set(PointerRNA *ptr, const char *value) {
	FileSelectParams *data = (FileSelectParams *)(ptr->data);
	LIB_strcpy(data->file, 256, value);
}

extern void FileSelectParams_filename_get(PointerRNA *ptr, char *value) {
	FileSelectParams *data = (FileSelectParams *)(ptr->data);
	strcpy(value, data->file);
}

extern int FileSelectParams_filename_length(PointerRNA *ptr) {
	FileSelectParams *data = (FileSelectParams *)(ptr->data);
	return strlen(data->file);
}

/* File Select Parameters */

CollectionPropertyRNA rna_FileSelectParams_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_FileSelectParams_title_,
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
	.begin = FileSelectParams_rna_properties_begin,
	.next = FileSelectParams_rna_properties_next,
	.end = FileSelectParams_rna_properties_end,
	.get = FileSelectParams_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_FileSelectParams_rna_properties = (PropertyRNA *)&rna_FileSelectParams_rna_properties_.property;

StringPropertyRNA rna_FileSelectParams_title_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_FileSelectParams_rna_properties_,
		.next = (PropertyRNA *)&rna_FileSelectParams_directory_,
		.magic = -1,
		.identifier = "title",
		.flag = PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Title",
		.description = "Title for the file browser",
		.arraydimension = 0,
		.arraylength = {64, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = FileSelectParams_title_get,
	.length = FileSelectParams_title_length,
	.set = NULL,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 64,
	.defaultvalue = "",
};

PropertyRNA *rna_FileSelectParams_title = (PropertyRNA *)&rna_FileSelectParams_title_.property;

StringPropertyRNA rna_FileSelectParams_directory_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_FileSelectParams_title_,
		.next = (PropertyRNA *)&rna_FileSelectParams_filename_,
		.magic = -1,
		.identifier = "directory",
		.flag = PROP_EDITABLE | PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Directory",
		.description = "Directory displayed in the file browser",
		.arraydimension = 0,
		.arraylength = {1024, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = FileSelectParams_directory_get,
	.length = FileSelectParams_directory_length,
	.set = FileSelectParams_directory_set,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 1024,
	.defaultvalue = "",
};

PropertyRNA *rna_FileSelectParams_directory = (PropertyRNA *)&rna_FileSelectParams_directory_.property;

StringPropertyRNA rna_FileSelectParams_filename_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_FileSelectParams_directory_,
		.next = NULL,
		.magic = -1,
		.identifier = "filename",
		.flag = PROP_EDITABLE | PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_FILENAME,
		.name = "File Name",
		.description = "Active file in the file browser",
		.arraydimension = 0,
		.arraylength = {256, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = FileSelectParams_filename_get,
	.length = FileSelectParams_filename_length,
	.set = FileSelectParams_filename_set,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 256,
	.defaultvalue = "",
};

PropertyRNA *rna_FileSelectParams_filename = (PropertyRNA *)&rna_FileSelectParams_filename_.property;

StructRNA RNA_FileSelectParams = {
	.container = {
		.prev = (ContainerRNA *)&RNA_PoseBone,
		.next = (ContainerRNA *)&RNA_OperatorProperties,
		/* rna_properties */
		/* title */
		/* directory */
		/* filename */
		.properties = {
			.first = (PropertyRNA *)&rna_FileSelectParams_rna_properties_,
			.last = (PropertyRNA *)&rna_FileSelectParams_filename_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "FileSelectParams",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "File Select Parameters",
	.description = "File Select Parameters",
	.nameproperty = NULL,
	.iteratorproperty = (PropertyRNA *)&rna_FileSelectParams_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = NULL,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

