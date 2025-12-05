#include "DNA_object_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

ROSE_INLINE void rna_def_pose_channel(RoseRNA *rna) {
	do {
		StructRNA *posebone = RNA_def_struct(rna, "PoseBone", NULL);
		RNA_def_struct_sdna(posebone, "PoseChannel");
		RNA_def_struct_ui_text(posebone, "Pose Bone", "A single bone in a pose, including settings for animating the bone");

		do {
			PropertyRNA *name = RNA_def_property(posebone, "name", PROP_STRING, PROP_NONE);
			RNA_def_property_ui_text(name, "Name", "");
			RNA_def_property_string_maxlength(name, ARRAY_SIZE(((PoseChannel *)NULL)->name));
			RNA_def_struct_name_property(posebone, name);
		} while (false);

		/* Transformation settings */

		do {
			PropertyRNA *location = RNA_def_property(posebone, "location", PROP_FLOAT, PROP_TRANSLATION);
			RNA_def_property_float_sdna(location, NULL, "loc");
			RNA_def_property_ui_text(location, "Location", "");
		} while (false);

		do {
			PropertyRNA *scale = RNA_def_property(posebone, "scale", PROP_FLOAT, PROP_TRANSLATION);
			RNA_def_property_float_array_default(scale, rna_default_scale_3d);
			RNA_def_property_ui_text(scale, "Scale", "");
		} while (false);

		do {
			PropertyRNA *quat = RNA_def_property(posebone, "quaternion", PROP_FLOAT, PROP_TRANSLATION);
			RNA_def_property_float_sdna(quat, NULL, "quat");
			RNA_def_property_float_array_default(quat, rna_default_quaternion);
			RNA_def_property_ui_text(quat, "Quaternion Rotation", "Rotation in Quaternions");
		} while (false);

		/* TODO: Add axis angle. */

		do {
			PropertyRNA *euler = RNA_def_property(posebone, "euler", PROP_FLOAT, PROP_TRANSLATION);
			RNA_def_property_float_sdna(euler, NULL, "euler");
			RNA_def_property_ui_text(euler, "Euler Rotation", "Rotation in Eulers");
			RNA_def_property_ui_range(euler, -FLT_MAX, FLT_MAX, 100, RNA_TRANSLATION_PREC_DEFAULT);
		} while (false);

	} while (false);
}

void RNA_def_Pose(RoseRNA *rna) {
	do {
		StructRNA *pose = RNA_def_struct(rna, "Pose", NULL);
		RNA_def_struct_ui_text(pose, "Pose", "A collection of pose channels, including settings for animating bones");

		do {
			PropertyRNA *bones = RNA_def_property(pose, "bones", PROP_COLLECTION, PROP_NONE);
			RNA_def_property_collection_sdna(bones, NULL, "channelbase", NULL);
			RNA_def_property_struct_type(bones, "PoseBone");
			RNA_def_property_ui_text(bones, "Pose Bones", "Individual pose bones for the armature");

			RNA_def_property_collection_funcs(bones, NULL, NULL, NULL, NULL, NULL, NULL, "rna_PoseBones_lookup_string");
		} while (false);
	} while (false);

	rna_def_pose_channel(rna);
}
