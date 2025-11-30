#include "MOD_modifiertypes.h"

ModifierTypeInfo MODType_NONE = {
	.idname = "None",
	.name = "None",
	.size = sizeof(ModifierData),
	.type = MODIFIER_TYPE_NONE,

	.copy_data = NULL,

	.deform_verts = NULL,

	.init_data = NULL,
	.free_data = NULL,
	.depends_on_time = NULL,
	.depends_on_normals = NULL,
	.foreach_ID_link = NULL,
	.free_runtime_data = NULL,
};
