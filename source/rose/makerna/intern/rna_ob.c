#include "DNA_object_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

void RNA_def_Object(RoseRNA *rna) {
	do {
		StructRNA *object = RNA_def_struct(rna, "Object", "ID");

		do {
			PropertyRNA *pose = RNA_def_property(object, "pose", PROP_POINTER, PROP_NONE);
			RNA_def_property_pointer_sdna(pose, NULL, "pose");
			RNA_def_property_struct_type(pose, "Pose");
			RNA_def_property_ui_text(pose, "Pose", "Current pose for armatures");
		} while (false);

		do {
			PropertyRNA *loc = RNA_def_property(object, "location", PROP_FLOAT, PROP_NONE);
			RNA_def_property_float_sdna(loc, NULL, "loc");
			RNA_def_property_ui_text(loc, "Location", "");
		} while (false);

		do {
			PropertyRNA *quat = RNA_def_property(object, "quaternion", PROP_FLOAT, PROP_NONE);
			RNA_def_property_float_sdna(quat, NULL, "quat");
			RNA_def_property_float_array_default(quat, rna_default_quaternion);
			RNA_def_property_ui_text(quat, "Quaternion Rotation", "Rotation in Quaternions");
		} while (false);

		/* TODO: Add axis angle. */

		do {
			PropertyRNA *euler = RNA_def_property(object, "euler", PROP_FLOAT, PROP_NONE);
			RNA_def_property_float_sdna(euler, NULL, "rot");
			RNA_def_property_ui_text(euler, "Euler Rotation", "Rotation in Eulers");
			RNA_def_property_ui_range(euler, -FLT_MAX, FLT_MAX, 100, RNA_TRANSLATION_PREC_DEFAULT);
		} while (false);

		do {
			PropertyRNA *scale = RNA_def_property(object, "scale", PROP_FLOAT, PROP_NONE);
			RNA_def_property_ui_text(scale, "Scale", "");
		} while (false);
	} while (false);
}
