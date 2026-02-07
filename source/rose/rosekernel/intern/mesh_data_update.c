#include "LIB_listbase.h"

#include "KER_deform.h"
#include "KER_object.h"
#include "KER_mesh.h"
#include "KER_modifier.h"
#include "KER_scene.h"

/* -------------------------------------------------------------------- */
/** \name Mesh Depsgraph Update
 * \{ */

ROSE_STATIC void mesh_calc_modifiers(Scene *scene, Object *object, const bool use_deform, const bool use_cache) {
	/* The final mesh is the result of calculating all enabled modifiers. */
	Mesh *mesh = (Mesh *)object->data;

	const ModifierEvalContext mectx = {
		.scene = scene,
		.object = object,
	};

	ModifierData *md = (ModifierData *)object->modifiers.first;

	/* Apply all leading deform modifiers. */
	if (use_deform) {
		for (; md; md = md->next) {
			const ModifierTypeInfo *mti = KER_modifier_get_info((eModifierType)md->type);

			if (!mti) {
				continue;
			}

			if (mti->type == OnlyDeform) {
				float(*positions)[3] = KER_mesh_vert_positions_for_write(mesh);

				KER_modifier_deform_verts(md, &mectx, mesh, positions, mesh->totvert);
			}
			else {
				break;
			}
		}
	}

	/* Apply all remaining constructive and deforming modifiers. */
	for (; md; md = md->next) {
		const ModifierTypeInfo *mti = KER_modifier_get_info((eModifierType)md->type);


		if (!mti) {
			return;
		}

		if (mti->type == OnlyDeform && !use_deform) {
			continue;
		}

		if (mti->type == OnlyDeform) {
			float(*positions)[3] = KER_mesh_vert_positions_for_write(mesh);

			KER_modifier_deform_verts(md, &mectx, mesh, positions, mesh->totvert);
		}
		else {
			// Apply other type of modifiers here!
		}
	}
}

ROSE_STATIC void mesh_build_data(Scene *scene, Object *object, Mesh *mesh) {
	mesh_calc_modifiers(scene, object, true, true);
}

void KER_mesh_copy_parameters(Mesh *me_dst, const Mesh *me_src) {
	me_dst->flag = me_src->flag;
}

void KER_mesh_copy_parameters_for_eval(Mesh *me_dst, const Mesh *me_src) {
	/* User counts aren't handled, don't copy into a mesh from #G_MAIN. */
	ROSE_assert(me_dst->id.tag & (ID_TAG_NO_MAIN | ID_TAG_COPIED_ON_WRITE));

	KER_mesh_copy_parameters(me_dst, me_src);
	KER_mesh_assert_normals_dirty_or_calculated(me_dst);

	/* Copy vertex group names. */
	ROSE_assert(LIB_listbase_is_empty(&me_dst->vertex_group_names));
	KER_defgroup_copy_list(&me_dst->vertex_group_names, &me_src->vertex_group_names);
}

void KER_mesh_data_update(Scene *scene, Object *object) {
	ROSE_assert(object->type == OB_MESH);

	Mesh *mesh = (Mesh *)object->data;

	mesh_build_data(scene, object, mesh);
}

/** \} */
