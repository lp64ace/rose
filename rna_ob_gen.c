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

extern PointerRNA Object_pose_get(PointerRNA *ptr) {
	Object *data = (Object *)(ptr->data);
	return RNA_pointer_create_with_parent(ptr, &RNA_Pose, data->pose);
}

extern void Object_location_get(PointerRNA *ptr, float values[3]) {
	Object *data = (Object *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		values[i] = (float)((data->loc)[i]);
	}
}

extern void Object_location_set(PointerRNA *ptr, const float values[3]) {
	Object *data = (Object *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		(data->loc)[i] = values[i];
	}
}

extern void Object_quaternion_get(PointerRNA *ptr, float values[4]) {
	Object *data = (Object *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 4; i++) {
		values[i] = (float)((data->quat)[i]);
	}
}

extern void Object_quaternion_set(PointerRNA *ptr, const float values[4]) {
	Object *data = (Object *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 4; i++) {
		(data->quat)[i] = values[i];
	}
}

extern void Object_euler_get(PointerRNA *ptr, float values[3]) {
	Object *data = (Object *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		values[i] = (float)((data->rot)[i]);
	}
}

extern void Object_euler_set(PointerRNA *ptr, const float values[3]) {
	Object *data = (Object *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		(data->rot)[i] = values[i];
	}
}

extern void Object_scale_get(PointerRNA *ptr, float values[3]) {
	Object *data = (Object *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		values[i] = (float)((data->scale)[i]);
	}
}

extern void Object_scale_set(PointerRNA *ptr, const float values[3]) {
	Object *data = (Object *)(ptr->data);
	unsigned int i;

	for (i = 0; i < 3; i++) {
		(data->scale)[i] = values[i];
	}
}

static PointerRNA ObjectBase_rna_properties_get(CollectionPropertyIterator *iter) {
	PropCollectionGetFunc fn = (PropCollectionGetFunc)rna_builtin_properties_get;
	return fn(iter);
}

extern void ObjectBase_rna_properties_begin(CollectionPropertyIterator *iter, PointerRNA *ptr) {
	memset(iter, 0, sizeof(CollectionPropertyIterator));
	iter->parent = *ptr;
	iter->property = (PropertyRNA *)&rna_ObjectBase_rna_properties_;

	PropCollectionBeginFunc fn = rna_builtin_properties_begin;
	fn(iter, ptr);

	if (iter->valid) {
		iter->ptr = ObjectBase_rna_properties_get(iter);
	}
}

extern void ObjectBase_rna_properties_next(CollectionPropertyIterator *iter) {
	PropCollectionNextFunc fn = rna_builtin_properties_next;
	fn(iter);

	if (iter->valid) {
		iter->ptr = ObjectBase_rna_properties_get(iter);
	}
}

extern void ObjectBase_rna_properties_end(CollectionPropertyIterator *iter) {
	PropCollectionEndFunc fn = rna_builtin_properties_end;
	fn(iter);
}

extern PointerRNA ObjectBase_object_get(PointerRNA *ptr) {
	Base *data = (Base *)(ptr->data);
	return RNA_pointer_create_with_parent(ptr, &RNA_Object, data->object);
}

/* Object */

PointerPropertyRNA rna_Object_pose_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_Object_location_,
		.magic = -1,
		.identifier = "pose",
		.flag = 0,
		.type = PROP_POINTER,
		.subtype = PROP_NONE,
		.name = "Pose",
		.description = "Current pose for armatures",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = Object_pose_get,
	.set = NULL,
	.type = NULL,
	.poll = NULL,
	.srna = &RNA_Pose,
};

PropertyRNA *rna_Object_pose = (PropertyRNA *)&rna_Object_pose_.property;

static float rna_Object_location_default[3] = {
	0.0f,
	0.0f,
	0.0f
};

FloatPropertyRNA rna_Object_location_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Object_pose_,
		.next = (PropertyRNA *)&rna_Object_quaternion_,
		.magic = -1,
		.identifier = "location",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE,
		.type = PROP_FLOAT,
		.subtype = PROP_NONE,
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
	.getarray = Object_location_get,
	.setarray = Object_location_set,
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
	.defaultarray = rna_Object_location_default
};

PropertyRNA *rna_Object_location = (PropertyRNA *)&rna_Object_location_.property;

static float rna_Object_quaternion_default[4] = {
	1.0f,
	0.0f,
	0.0f,
	0.0f
};

FloatPropertyRNA rna_Object_quaternion_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Object_location_,
		.next = (PropertyRNA *)&rna_Object_euler_,
		.magic = -1,
		.identifier = "quaternion",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE,
		.type = PROP_FLOAT,
		.subtype = PROP_NONE,
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
	.getarray = Object_quaternion_get,
	.setarray = Object_quaternion_set,
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
	.defaultarray = rna_Object_quaternion_default
};

PropertyRNA *rna_Object_quaternion = (PropertyRNA *)&rna_Object_quaternion_.property;

static float rna_Object_euler_default[3] = {
	0.0f,
	0.0f,
	0.0f
};

FloatPropertyRNA rna_Object_euler_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Object_quaternion_,
		.next = (PropertyRNA *)&rna_Object_scale_,
		.magic = -1,
		.identifier = "euler",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE,
		.type = PROP_FLOAT,
		.subtype = PROP_NONE,
		.name = "Euler Rotation",
		.description = "Rotation in Eulers",
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
	.getarray = Object_euler_get,
	.setarray = Object_euler_set,
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
	.defaultarray = rna_Object_euler_default
};

PropertyRNA *rna_Object_euler = (PropertyRNA *)&rna_Object_euler_.property;

static float rna_Object_scale_default[3] = {
	0.0f,
	0.0f,
	0.0f
};

FloatPropertyRNA rna_Object_scale_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_Object_euler_,
		.next = NULL,
		.magic = -1,
		.identifier = "scale",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE,
		.type = PROP_FLOAT,
		.subtype = PROP_NONE,
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
	.getarray = Object_scale_get,
	.setarray = Object_scale_set,
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
	.defaultarray = rna_Object_scale_default
};

PropertyRNA *rna_Object_scale = (PropertyRNA *)&rna_Object_scale_.property;

StructRNA RNA_Object = {
	.container = {
		.prev = (ContainerRNA *)&RNA_PropertyGroup,
		.next = (ContainerRNA *)&RNA_ObjectBase,
		/* pose */
		/* location */
		/* quaternion */
		/* euler */
		/* scale */
		.properties = {
			.first = (PropertyRNA *)&rna_Object_pose_,
			.last = (PropertyRNA *)&rna_Object_scale_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "Object",
	.flag = STRUCT_ID | STRUCT_ID_REFCOUNT | STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Object",
	.description = "",
	.nameproperty = (PropertyRNA *)&rna_ID_name_,
	.iteratorproperty = (PropertyRNA *)&rna_ID_rna_properties_,
	.base = &RNA_ID,
	.nested = NULL,
	.refine = rna_ID_refine,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

/* Object Base */

CollectionPropertyRNA rna_ObjectBase_rna_properties_ = {
	.property = {
		.prev = NULL,
		.next = (PropertyRNA *)&rna_ObjectBase_object_,
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
	.begin = ObjectBase_rna_properties_begin,
	.next = ObjectBase_rna_properties_next,
	.end = ObjectBase_rna_properties_end,
	.get = ObjectBase_rna_properties_get,
	.length = NULL,
	.lookupint = NULL,
	.lookupstring = rna_builtin_properties_lookup_string,
	.element = &RNA_Property,
};

PropertyRNA *rna_ObjectBase_rna_properties = (PropertyRNA *)&rna_ObjectBase_rna_properties_.property;

PointerPropertyRNA rna_ObjectBase_object_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_ObjectBase_rna_properties_,
		.next = (PropertyRNA *)&rna_ObjectBase_select_,
		.magic = -1,
		.identifier = "object",
		.flag = PROP_PTR_NO_OWNERSHIP,
		.type = PROP_POINTER,
		.subtype = PROP_NONE,
		.name = "Object",
		.description = "Object this base links to",
		.arraydimension = 0,
		.arraylength = {0, 0, 0},
		.totarraylength = 0,
		.flagex = 0,
		.update = NULL,
		.rawoffset = 0,
		.rawtype = PROP_RAW_UNSET,
		.srna = NULL,
	},
	.get = ObjectBase_object_get,
	.set = NULL,
	.type = NULL,
	.poll = NULL,
	.srna = &RNA_Object,
};

PropertyRNA *rna_ObjectBase_object = (PropertyRNA *)&rna_ObjectBase_object_.property;

BoolPropertyRNA rna_ObjectBase_select_ = {
	.property = {
		.prev = (PropertyRNA *)&rna_ObjectBase_object_,
		.next = NULL,
		.magic = -1,
		.identifier = "select",
		.flag = PROP_ANIMATABLE | PROP_EDITABLE,
		.type = PROP_BOOLEAN,
		.subtype = PROP_NONE,
		.name = "Select",
		.description = "Object base selection state",
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

PropertyRNA *rna_ObjectBase_select = (PropertyRNA *)&rna_ObjectBase_select_.property;

StructRNA RNA_ObjectBase = {
	.container = {
		.prev = (ContainerRNA *)&RNA_Object,
		.next = (ContainerRNA *)&RNA_Pose,
		/* rna_properties */
		/* object */
		/* select */
		.properties = {
			.first = (PropertyRNA *)&rna_ObjectBase_rna_properties_,
			.last = (PropertyRNA *)&rna_ObjectBase_select_,
		},
		.property_identifier_lookup_set = NULL,
	},
	.identifier = "ObjectBase",
	.flag = STRUCT_UNDO | STRUCT_PUBLIC_NAMESPACE,
	.name = "Object Base",
	.description = "An object instance in a render layer",
	.nameproperty = NULL,
	.iteratorproperty = (PropertyRNA *)&rna_ObjectBase_rna_properties_,
	.base = NULL,
	.nested = NULL,
	.refine = NULL,
	.path = NULL,
	.idproperties = NULL,
	.system_idproperties = NULL,
};

