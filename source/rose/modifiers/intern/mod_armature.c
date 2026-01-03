#include "MOD_modifiertypes.h"

#include "LIB_listbase.h"

#include "KER_armature.h"
#include "KER_lib_query.h"
#include "KER_object.h"
#include "KER_mesh.h"

ROSE_STATIC void deform_foreach_id(ModifierData *md, Object *ob, IDWalkFunc walk, void *user_data) {
	ArmatureModifierData *amd = (ArmatureModifierData *)md;

	walk(user_data, ob, (ID **)&amd->object, IDWALK_CB_NOP);
}

ROSE_STATIC void deform_verts(ModifierData *md, const ModifierEvalContext *ctx, Mesh *mesh, float (*positions)[3], size_t length) {
	ArmatureModifierData *amd = (ArmatureModifierData *)md;

	KER_armature_deform_coords_with_mesh(amd->object, ctx->object, positions, length, mesh);
}

ModifierTypeInfo MODType_ARMATURE = {
	.idname = "Armature",
	.name = "Armature",
	.size = sizeof(ArmatureModifierData),

	.type = OnlyDeform,
	.flag = 0,

	.copy_data = NULL,

	.deform_verts = deform_verts,

	.init_data = NULL,
	.free_data = NULL,
	.depends_on_time = NULL,
	.depends_on_normals = NULL,
	.foreach_ID_link = NULL,
	.free_runtime_data = NULL,
};
