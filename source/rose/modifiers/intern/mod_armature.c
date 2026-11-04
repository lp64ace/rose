#include "MOD_modifiertypes.h"

#include "LIB_listbase.h"

#include "DEG_depsgraph_build.h"

#include "KER_armature.h"
#include "KER_lib_query.h"
#include "KER_object.h"
#include "KER_mesh.h"

void KER_armature_modifier_copy_data(const ArmatureModifierData *source, ArmatureModifierData *target, int flag) {
	target->object = source->object;
}

ROSE_STATIC void deform_copy_data(const ModifierData *source, ModifierData *target, int flag) {
	KER_armature_modifier_copy_data((const ArmatureModifierData *)source, (ArmatureModifierData *)target, flag);
}

ROSE_STATIC void deform_verts(ModifierData *md, const ModifierEvalContext *ctx, Mesh *mesh, float (*positions)[3], size_t length) {
	ArmatureModifierData *amd = (ArmatureModifierData *)md;

	KER_armature_deform_coords_with_mesh(amd->object, ctx->object, positions, length, mesh);
}

ROSE_STATIC void update_depsgraph(ModifierData *md, const ModifierUpdateDepsgraphContext *ctx) {
	ArmatureModifierData *amd = (ArmatureModifierData *)md;

	if ((md->flag & MODIFIER_DEVICE_ONLY) != 0) {
		return;
	}

	if (amd->object != NULL) {
		DEG_add_object_relation(ctx->node, amd->object, DEG_OB_COMP_EVAL_POSE, "Armature Modifier");
		DEG_add_object_relation(ctx->node, amd->object, DEG_OB_COMP_TRANSFORM, "Armature Modifier");
	}
	DEG_add_depends_on_transform_relation(ctx->node, "Armature Modifier");
}

ROSE_STATIC void deform_foreach_id(ModifierData *md, Object *ob, IDWalkFunc walk, void *user_data) {
	ArmatureModifierData *amd = (ArmatureModifierData *)md;

	walk(user_data, ob, (ID **)&amd->object, IDWALK_CB_NOP);
}

ModifierTypeInfo MODType_ARMATURE = {
	.idname = "Armature",
	.name = "Armature",
	.dnastruct = "ArmatureModifierData",
	.size = sizeof(ArmatureModifierData),

	.type = OnlyDeform,
	.flag = 0,

	.copy_data = deform_copy_data,

	.deform_verts = deform_verts,

	.init_data = NULL,
	.free_data = NULL,
	.update_depsgraph = update_depsgraph,
	.depends_on_time = NULL,
	.depends_on_normals = NULL,
	.foreach_ID_link = deform_foreach_id,
	.free_runtime_data = NULL,
};
