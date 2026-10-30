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

static PointerRNA Pose_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void Pose_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_Pose_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = Pose_rna_properties_get(iter);
	}
}

extern void Pose_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = Pose_rna_properties_get(iter);
	}
}

extern void Pose_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

static PointerRNA Pose_bone_get(CollectionPropertyIterator *iter) {
	return RNA_pointer_create_with_parent(&iter->parent, &RNA_PoseBone, rna_iterator_listbase_get(iter));
}

extern void Pose_bone_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	Pose *data = (Pose *)(ptr->data);

	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_Pose_bone_;

	rna_iterator_listbase_begin(iter, ptr, &data->channelbase, NULL);

	if (iter->valid) {
		iter->ptr = Pose_bone_get(iter);
	}
}

extern void Pose_bone_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_iterator_listbase_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = Pose_bone_get(iter);
	}
}

extern void Pose_bone_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_iterator_listbase_end;
	fn(iter);
}

static PointerRNA PoseBone_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void PoseBone_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_PoseBone_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = PoseBone_rna_properties_get(iter);
	}
}

extern void PoseBone_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = PoseBone_rna_properties_get(iter);
	}
}

extern void PoseBone_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

extern void PoseBone_name_set(PointerRNA *ptr, const char *value) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	LIB_strcpy(data->name, 64, value);
}

extern void PoseBone_name_get(PointerRNA *ptr, char *value) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	strcpy(value, data->name);
}

extern int PoseBone_name_length(PointerRNA *ptr) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	return strlen(data->name);
}

extern void PoseBone_location_get(PointerRNA *ptr, float values[3]) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		values[i] = (float)((data->loc)[i]);
	}
}

extern void PoseBone_location_set(PointerRNA *ptr, const float values[3]) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		(data->loc)[i] = values[i];
	}
}

extern void PoseBone_scale_get(PointerRNA *ptr, float values[3]) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		values[i] = (float)((data->scale)[i]);
	}
}

extern void PoseBone_scale_set(PointerRNA *ptr, const float values[3]) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		(data->scale)[i] = values[i];
	}
}

extern void PoseBone_quaternion_get(PointerRNA *ptr, float values[4]) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 4; i++) {
		values[i] = (float)((data->quat)[i]);
	}
}

extern void PoseBone_quaternion_set(PointerRNA *ptr, const float values[4]) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 4; i++) {
		(data->quat)[i] = values[i];
	}
}

extern void PoseBone_euler_get(PointerRNA *ptr, float values[3]) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		values[i] = (float)((data->euler)[i]);
	}
}

extern void PoseBone_euler_set(PointerRNA *ptr, const float values[3]) {
	PoseChannel *data = (PoseChannel *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		(data->euler)[i] = values[i];
	}
}

/* Pose */

CollectionPropertyRNA rna_Pose_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_Pose_bone_,
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
	.begin = Pose_rna_properties_begin,
	.next = Pose_rna_properties_next,
	.end = Pose_rna_properties_end,
	.get = Pose_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_Pose_rna_properties = (PropertyRNA *)&rna_Pose_rna_properties_.property;

CollectionPropertyRNA rna_Pose_bone_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Pose_rna_properties_,
		.next = NULL,
		.magic = -1,
		.identifier = "bone",
		.flag = 0,
		.type = PROP_COLLECTION,
		.subtype = PROP_NONE,
		.name = "Pose Bones",
		.description = "Individual pose bones for the armature",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.begin = Pose_bone_begin,
	.next = Pose_bone_next,
	.end = Pose_bone_end,
	.get = Pose_bone_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_PoseBones_lookup_string,
	.element = &RNA_PoseBone,
};

PropertyRNA *rna_Pose_bone = (PropertyRNA *)&rna_Pose_bone_.property;

StructRNA RNA_Pose = {
	.container = {
		.prev = (ContainerRNA *)&RNA_ObjectBase,
		.next = (ContainerRNA *)&RNA_PoseBone,
		/* rna_properties */
		/* bone */
		.properties = {
			.first = (PropertyRNA *)&rna_Pose_rna_properties_,
			.last = (PropertyRNA *)&rna_Pose_bone_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "Pose",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Pose",
	.description = "A collection of pose channels, including settings for animating bones",
	.nameproperty = NULL,
	.iteratorproperty = (PropertyRNA *)&rna_Pose_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = NULL,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

/* Pose Bone */

CollectionPropertyRNA rna_PoseBone_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_PoseBone_name_,
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
	.begin = PoseBone_rna_properties_begin,
	.next = PoseBone_rna_properties_next,
	.end = PoseBone_rna_properties_end,
	.get = PoseBone_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_PoseBone_rna_properties = (PropertyRNA *)&rna_PoseBone_rna_properties_.property;

StringPropertyRNA rna_PoseBone_name_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PoseBone_rna_properties_,
		.next = (PropertyRNA *)&rna_PoseBone_location_,
		.magic = -1,
		.identifier = "name",
		.flag = PROP_EDITABLE | PROP_NEVER_NULL,
		.type = PROP_STRING,
		.subtype = PROP_NONE,
		.name = "Name",
		.description = "",
		.arraydimension = 0,
		.arraylength = {64, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = PoseBone_name_get,
	.length = PoseBone_name_length,
	.set = PoseBone_name_set,
	.get_ex = NULL,
	.length_ex = NULL,
	.set_ex = NULL,
	.maxlength = 64,
	.defaultvalue = "",
};

PropertyRNA *rna_PoseBone_name = (PropertyRNA *)&rna_PoseBone_name_.property;

static float rna_PoseBone_location_default[3] = {
	0.0f,
	0.0f,
	0.0f
};

FloatPropertyRNA rna_PoseBone_location_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PoseBone_name_,
		.next = (PropertyRNA *)&rna_PoseBone_scale_,
		.magic = -1,
		.identifier = "location",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE,
		.type = PROP_FLOAT,
		.subtype = PROP_TRANSLATION,
		.name = "Location",
		.description = "",
		.arraydimension = 1,
		.arraylength = {3, 0, 0},
		.totarraylength = 3,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = PoseBone_location_get,
	.setarray = PoseBone_location_set,
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
	.defaultarray = rna_PoseBone_location_default
};

PropertyRNA *rna_PoseBone_location = (PropertyRNA *)&rna_PoseBone_location_.property;

static float rna_PoseBone_scale_default[3] = {
	1.0f,
	1.0f,
	1.0f
};

FloatPropertyRNA rna_PoseBone_scale_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PoseBone_location_,
		.next = (PropertyRNA *)&rna_PoseBone_quaternion_,
		.magic = -1,
		.identifier = "scale",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE,
		.type = PROP_FLOAT,
		.subtype = PROP_TRANSLATION,
		.name = "Scale",
		.description = "",
		.arraydimension = 1,
		.arraylength = {3, 0, 0},
		.totarraylength = 3,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = PoseBone_scale_get,
	.setarray = PoseBone_scale_set,
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
	.defaultarray = rna_PoseBone_scale_default
};

PropertyRNA *rna_PoseBone_scale = (PropertyRNA *)&rna_PoseBone_scale_.property;

static float rna_PoseBone_quaternion_default[4] = {
	1.0f,
	0.0f,
	0.0f,
	0.0f
};

FloatPropertyRNA rna_PoseBone_quaternion_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PoseBone_scale_,
		.next = (PropertyRNA *)&rna_PoseBone_euler_,
		.magic = -1,
		.identifier = "quaternion",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE,
		.type = PROP_FLOAT,
		.subtype = PROP_TRANSLATION,
		.name = "Quaternion Rotation",
		.description = "Rotation in Quaternions",
		.arraydimension = 1,
		.arraylength = {4, 0, 0},
		.totarraylength = 4,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = PoseBone_quaternion_get,
	.setarray = PoseBone_quaternion_set,
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
	.defaultarray = rna_PoseBone_quaternion_default
};

PropertyRNA *rna_PoseBone_quaternion = (PropertyRNA *)&rna_PoseBone_quaternion_.property;

static float rna_PoseBone_euler_default[3] = {
	0.0f,
	0.0f,
	0.0f
};

FloatPropertyRNA rna_PoseBone_euler_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_PoseBone_quaternion_,
		.next = NULL,
		.magic = -1,
		.identifier = "euler",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE,
		.type = PROP_FLOAT,
		.subtype = PROP_TRANSLATION,
		.name = "Euler Rotation",
		.description = "Rotation in Eulers",
		.arraydimension = 1,
		.arraylength = {3, 0, 0},
		.totarraylength = 3,
		.flagex = 0,
		.update = rna_Pose_update,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = NULL,
	.set = NULL,
	.getarray = PoseBone_euler_get,
	.setarray = PoseBone_euler_set,
	.range = NULL,
	.get_ex = NULL,
	.set_ex = NULL,
	.getarray_ex = NULL,
	.setarray_ex = NULL,
	.range_ex = NULL,
	.softmin = -FLT_MAX,
	.softmax = FLT_MAX,
	.hardmin = -FLT_MAX,
	.hardmax = FLT_MAX,
	.step = 100.0f,
	.precision = 3,
	.get_default = NULL,
	.get_default_array = NULL,
	.defaultvalue = 0.0f,
	.defaultarray = rna_PoseBone_euler_default
};

PropertyRNA *rna_PoseBone_euler = (PropertyRNA *)&rna_PoseBone_euler_.property;

StructRNA RNA_PoseBone = {
	.container = {
		.prev = (ContainerRNA *)&RNA_Pose,
		.next = (ContainerRNA *)&RNA_FileSelectParams,
		/* rna_properties */
		/* name */
		/* location */
		/* scale */
		/* quaternion */
		/* euler */
		.properties = {
			.first = (PropertyRNA *)&rna_PoseBone_rna_properties_,
			.last = (PropertyRNA *)&rna_PoseBone_euler_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "PoseBone",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Pose Bone",
	.description = "A single bone in a pose, including settings for animating the bone",
	.nameproperty = (PropertyRNA *)&rna_PoseBone_name_,
	.iteratorproperty = (PropertyRNA *)&rna_PoseBone_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = NULL,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

