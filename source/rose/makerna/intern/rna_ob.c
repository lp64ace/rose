#include "DNA_object_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

void RNA_def_Object(RoseRNA *rna) {
	do {
		StructRNA *id = RNA_def_struct(rna, "Object", "ID");

		/* pose */
		do {
			PropertyRNA *pose = RNA_def_property(id, "pose", PROP_POINTER, PROP_NONE);
			RNA_def_property_pointer_sdna(pose, NULL, "pose");
			RNA_def_property_struct_type(pose, "Pose");
			RNA_def_property_ui_text(pose, "Pose", "Current pose for armatures");
		} while (false);
	} while (false);
}
