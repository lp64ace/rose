#include "KER_derived_mesh.h"
#include "KER_layer.h"
#include "KER_mesh.hh"
#include "KER_mesh_types.hh"
#include "KER_modifier.h"
#include "KER_object.h"
#include "KER_scene.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

ROSE_INLINE void object_get_datamask(Depsgraph *depsgraph, Object *ob, CustomData_MeshMasks *r_mask, bool *r_need_mapping) {
	ViewLayer *view_layer = DEG_get_evaluated_view_layer(depsgraph);

	DEG_get_customdata_mask_for_object(depsgraph, ob, r_mask);

	if (r_need_mapping) {
		*r_need_mapping = false;
	}

	/* Must never access original objects when dependency graph is not active: it might be already
	 * freed. */
	if (!DEG_is_active(depsgraph)) {
		return;
	}

	Object *actob = view_layer->active ? DEG_get_original_object(view_layer->active->object) : NULL;
	if (DEG_get_original_object(ob) == actob) {
		// Not needed currently!
	}

	r_mask->vmask |= CD_MASK_MDEFORMVERT;
}

ROSE_INLINE void mesh_calc_modifiers(Depsgraph *depsgraph, Scene *scene, Object *ob, const bool use_deform, const bool need_mapping, const CustomData_MeshMasks *mask, const bool use_cache, const bool allow_shared_mesh, Mesh **r_deform, Mesh **r_final) {
	/* Input and final mesh. Final mesh is only created the moment the first
	 * constructive modifier is executed, or a deform modifier needs normals
	 * or certain data layers. */
	Mesh *mesh_input = (Mesh *)ob->data;
	KER_mesh_assert_normals_dirty_or_calculated(mesh_input);
	Mesh *mesh_final = NULL;
	Mesh *mesh_deform = NULL;

	ROSE_assert((mesh_input->id.tag & ID_TAG_COPIED_ON_WRITE_EVAL_RESULT) == 0);

	/* Deformed vertex locations array. Deform only modifier need this type of
	 * float array rather than MVert*. Tracked along with mesh_final as an
	 * optimization to avoid copying coordinates back and forth if there are
	 * multiple sequential deform only modifiers. */
	float (*deformed_verts)[3] = NULL;
	int num_deformed_verts = mesh_input->totvert;
	bool is_prev_deform = false;

	ModifierEvalContext mectx;
	mectx.object = ob;
	mectx.scene = scene;

	ModifierData *md = static_cast<ModifierData *>(ob->modifiers.first);

	/* Apply all leading deform modifiers. */
	if (use_deform) {
		for (; md; md = md->next) {
			const ModifierTypeInfo *mti = KER_modifier_get_info((eModifierType)md->type);

			if (mti->type != OnlyDeform) {
				break;
			}

			if (md->flag & MODIFIER_DEVICE_ONLY) {
				continue;
			}

			// TODO!

			ROSE_assert_unreachable();
		}

		/* Result of all leading deforming modifiers is cached for
		 * places that wish to use the original mesh but with deformed
		 * coordinates (like vertex paint). */
		if (r_deform) {
			mesh_deform = KER_mesh_copy_for_eval(mesh_input, true);

			if (deformed_verts) {
				ROSE_assert_unreachable();
			}
		}
	}

	/* Apply all remaining constructive and deforming modifiers. */
	bool have_non_onlydeform_modifiers_appled = false;
	for (; md; md = md->next) {
		const ModifierTypeInfo *mti = KER_modifier_get_info((eModifierType)md->type);

		if (mti->type == OnlyDeform && !use_deform) {
			continue;
		}

		if (md->flag & MODIFIER_DEVICE_ONLY) {
			continue;
		}

		// TODO!

		ROSE_assert_unreachable();
	}

	/* Yay, we are done. If we have a Mesh and deformed vertices,
	 * we need to apply these back onto the Mesh. If we have no
	 * Mesh then we need to build one. */
	if (mesh_final == NULL) {
		if (deformed_verts == NULL && allow_shared_mesh) {
			mesh_final = mesh_input;
		}
		else {
			mesh_final = KER_mesh_copy_for_eval(mesh_input, true);
		}
	}
	if (deformed_verts) {
		ROSE_assert_unreachable();
	}

	/* Denotes whether the object which the modifier stack came from owns the mesh or whether the
	 * mesh is shared across multiple objects since there are no effective modifiers. */
	const bool is_own_mesh = (mesh_final == mesh_input);
	if (is_own_mesh) {
		rose::kernel::MeshRuntime *runtime = mesh_input->runtime;
		if (runtime->mesh_eval == NULL) {
				/* Not yet finalized by any instance, do it now
				 * Isolate since computing normals is multithreaded and we are holding a lock. */
				rose::threading::isolate_task([&] {
					mesh_final = KER_mesh_copy_for_eval(mesh_input, true);
					runtime->mesh_eval = mesh_final;
				});
		}
		else {
			/* Already finalized by another instance, reuse. */
			mesh_final = runtime->mesh_eval;
		}
	}

	/* Return final mesh */
	*r_final = mesh_final;
	if (r_deform) {
		*r_deform = mesh_deform;
	}
}

ROSE_INLINE void mesh_build_data(Depsgraph *depsgraph, Scene *scene, Object *ob, const CustomData_MeshMasks *mask, bool need_mapping) {
	Mesh *mesh_eval = NULL, *mesh_eval_deform = NULL;

	mesh_calc_modifiers(depsgraph, scene, ob, true, need_mapping, mask, true, true, &mesh_eval_deform, &mesh_eval);
	/* The modifier stack evaluation is storing result in mesh->runtime.mesh_eval, but this result
	 * is not guaranteed to be owned by object.
	 *
	 * Check ownership now, since later on we can not go to a mesh owned by someone else via
	 * object's runtime: this could cause access freed data on depsgraph destruction (mesh who owns
	 * the final result might be freed prior to object). */
	Mesh *mesh = (Mesh *)ob->data;
	const bool is_mesh_eval_owned = (mesh_eval != mesh->runtime->mesh_eval);
	KER_object_eval_assign_data(ob, &mesh_eval->id, is_mesh_eval_owned);

	ob->runtime.mesh_eval_deform = mesh_eval_deform;
}

void KER_derived_mesh_make(Depsgraph *depsgraph, Scene *scene, Object *ob, const CustomData_MeshMasks *mask) {
	ROSE_assert(ob->type == OB_MESH);
	/* Evaluated meshes aren't supposed to be created on original instances. If you do,
	 * they aren't cleaned up properly on mode switch, causing crashes, e.g T58150. */
	ROSE_assert(ob->id.tag & ID_TAG_COPIED_ON_WRITE);

	KER_object_free_derived_caches(ob);

	bool need_mapping;
	CustomData_MeshMasks cddata_masks = *mask;
	object_get_datamask(depsgraph, ob, &cddata_masks, &need_mapping);

	mesh_build_data(depsgraph, scene, ob, &cddata_masks, need_mapping);
}
