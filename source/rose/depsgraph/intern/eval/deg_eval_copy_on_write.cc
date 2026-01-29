#include "intern/depsgraph.hh"
#include "intern/builder/deg_builder.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/eval/deg_eval_copy_on_write.hh"
#include "intern/eval/deg_eval_runtime_backup.hh"

#include "KER_armature.h"
#include "KER_anim_data.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_object.h"
#include "KER_scene.h"

#include "LIB_utildefines.h"

namespace rose::depsgraph {

namespace {

struct RemapCallbackUserData {
	/* Dependency graph for which remapping is happening. */
	const Depsgraph *depsgraph;
};

int foreach_libblock_remap_callback(LibraryIDLinkCallbackData *cb_data) {
	ID **id_p = cb_data->self_ptr;
	if (*id_p == nullptr) {
		return IDWALK_RET_NOP;
	}

	RemapCallbackUserData *user_data = (RemapCallbackUserData *)cb_data->user_data;
	const Depsgraph *depsgraph = user_data->depsgraph;
	ID *id_orig = *id_p;
	if (deg_copy_on_write_is_needed(id_orig)) {
		ID *id_cow = depsgraph->get_cow_id(id_orig);
		ROSE_assert(id_cow != nullptr);
		*id_p = id_cow;
	}
	return IDWALK_RET_NOP;
}

/* Similar to generic BKE_id_copy() but does not require main and assumes pointer
 * is already allocated. */
bool id_copy_inplace_no_main(const ID *id, ID *newid) {
	return KER_id_copy_ex(nullptr, (ID *)id, &newid, LIB_ID_CREATE_NO_ALLOCATE | LIB_ID_CREATE_NO_MAIN) != nullptr;
}

bool scene_copy_inplace_no_main(const Scene *scene, Scene *new_scene) {
	return KER_id_copy_ex(nullptr, (ID *)&scene->id, (ID **)&new_scene, LIB_ID_CREATE_NO_ALLOCATE | LIB_ID_CREATE_NO_MAIN) != nullptr;
}

/* For the given scene get view layer which corresponds to an original for the
 * scene's evaluated one. This depends on how the scene is pulled into the
 * dependency graph. */
ViewLayer *get_original_view_layer(const Depsgraph *depsgraph, const IDNode *id_node) {
	if (id_node->linked_state == DEG_ID_LINKED_DIRECTLY) {
		return depsgraph->view_layer;
	}
	if (id_node->linked_state == DEG_ID_LINKED_VIA_SET) {
		Scene *scene_orig = reinterpret_cast<Scene *>(id_node->id_orig);
		return KER_view_layer_default_render(scene_orig);
	}
	/* Is possible to have scene linked indirectly (i.e. via the driver) which
	 * we need to support. Currently there are issues somewhere else, which
	 * makes testing hard. This is a reported problem, so will eventually be
	 * properly fixed.
	 *
	 * TODO(sergey): Support indirectly linked scene. */
	return nullptr;
}

void view_layer_update_orig_base_pointers(const ViewLayer *view_layer_orig, ViewLayer *view_layer_eval) {
	if (view_layer_orig == nullptr || view_layer_eval == nullptr) {
		/* Happens when scene is only used for parameters or compositor/sequencer. */
		return;
	}
	Base *base_orig = reinterpret_cast<Base *>(view_layer_orig->bases.first);
	LISTBASE_FOREACH(Base *, base_eval, &view_layer_eval->bases) {
		base_eval->runtime.base_orig = base_orig;
		base_orig = base_orig->next;
	}
}

template<typename T, typename R> void update_list_orig_pointers(const ListBase *listbase_orig, ListBase *listbase, T *R::*orig_field) {
	T *element_orig = reinterpret_cast<T *>(listbase_orig->first);
	T *element_cow = reinterpret_cast<T *>(listbase->first);

	/* Both lists should have the same number of elements, so the check on
	 * `element_cow` is just to prevent a crash if this is not the case. */
	while (element_orig != nullptr && element_cow != nullptr) {
		element_cow->runtime.*orig_field = element_orig;
		element_cow = element_cow->next;
		element_orig = element_orig->next;
	}

	ROSE_assert((element_orig == nullptr && element_cow == nullptr) || !"list of pointers of different sizes, unable to reliably set orig pointer");
}

void update_pose_orig_pointers(const Pose *pose_orig, Pose *pose_cow) {
	update_list_orig_pointers<PoseChannel, PoseChannel_Runtime>(&pose_orig->channelbase, &pose_cow->channelbase, &PoseChannel_Runtime::orig_pchannel);
}

/* Makes it so given view layer only has bases corresponding to enabled
 * objects. */
void view_layer_remove_disabled_bases(const Depsgraph *depsgraph, ViewLayer *view_layer) {
	if (view_layer == nullptr) {
		return;
	}
	ListBase enabled_bases = {nullptr, nullptr};
	LISTBASE_FOREACH_MUTABLE(Base *, base, &view_layer->bases) {
		/* TODO(sergey): Would be cool to optimize this somehow, or make it so
		 * builder tags bases.
		 *
		 * NOTE: The idea of using id's tag and check whether its copied ot not
		 * is not reliable, since object might be indirectly linked into the
		 * graph.
		 *
		 * NOTE: We are using original base since the object which evaluated base
		 * points to is not yet copied. This is dangerous access from evaluated
		 * domain to original one, but this is how the entire copy-on-write works:
		 * it does need to access original for an initial copy. */
		const bool is_object_enabled = deg_check_base_in_depsgraph(depsgraph, base);
		if (is_object_enabled) {
			LIB_addtail(&enabled_bases, base);
		}
		else {
			if (base == view_layer->active) {
				view_layer->active = nullptr;
			}
			MEM_freeN(base);
		}
	}
	view_layer->bases = enabled_bases;
}

void scene_setup_view_layers_before_remap(const Depsgraph *depsgraph, const IDNode *id_node, Scene *scene_cow) {
	// scene_remove_unused_view_layers(depsgraph, id_node, scene_cow);
}

void scene_setup_view_layers_after_remap(const Depsgraph *depsgraph, const IDNode *id_node, Scene *scene_cow) {
	const ViewLayer *view_layer_orig = get_original_view_layer(depsgraph, id_node);
	ViewLayer *view_layer_eval = reinterpret_cast<ViewLayer *>(scene_cow->view_layers.first);
	view_layer_update_orig_base_pointers(view_layer_orig, view_layer_eval);
	view_layer_remove_disabled_bases(depsgraph, view_layer_eval);
	/* TODO(sergey): Remove objects from collections as well.
	 * Not a HUGE deal for now, nobody is looking into those CURRENTLY.
	 * Still not an excuse to have those. */
}

/**
 * Do some special treatment of data transfer from original ID to its
 * CoW complementary part.
 *
 * Only use for the newly created CoW data-blocks.
 */
void update_id_after_copy(const Depsgraph *depsgraph, const IDNode *id_node, const ID *id_orig, ID *id_cow) {
	const ID_Type type = GS(id_orig->name);
	switch (type) {
		case ID_OB: {
			/* Ensure we don't drag someone's else derived mesh to the
			 * new copy of the object. */
			Object *object_cow = (Object *)id_cow;
			const Object *object_orig = (const Object *)id_orig;
			object_cow->runtime.data_orig = (ID *)object_cow->data;
			if (object_cow->type == OB_ARMATURE) {
				const Armature *armature_orig = (Armature *)object_orig->data;
				Armature *armature_cow = (Armature *)object_cow->data;
				KER_pose_remap_bone_pointers(armature_cow, object_cow->pose);
				update_pose_orig_pointers(object_orig->pose, object_cow->pose);
				KER_pose_channel_index_rebuild(object_cow->pose);
			}
			break;
		}
		case ID_SCE: {
			Scene *scene_cow = (Scene *)id_cow;
			const Scene *scene_orig = (const Scene *)id_orig;
			scene_setup_view_layers_after_remap(depsgraph, id_node, reinterpret_cast<Scene *>(id_cow));
			break;
		}
		default:
			break;
	}
}

/* Check whether given ID is expanded or still a shallow copy. */
inline bool check_datablock_expanded(const ID *id_cow) {
	return (id_cow->name[0] != '\0');
}

struct ValidateData {
	bool is_valid;
};

/**
 * This callback is used to validate that all nested ID data-blocks are
 * properly expanded.
 */
int foreach_libblock_validate_callback(LibraryIDLinkCallbackData *cb_data) {
	ValidateData *data = (ValidateData *)cb_data->user_data;
	ID **id_p = cb_data->self_ptr;

	if (*id_p != nullptr) {
		if (!check_datablock_expanded(*id_p)) {
			data->is_valid = false;
			/* TODO(sergey): Store which is not valid? */
		}
	}
	return IDWALK_RET_NOP;
}

/**
 * Actual implementation of logic which "expands" all the data which was not
 * yet copied-on-write.
 *
 * NOTE: Expects that CoW datablock is empty.
 */
ID *deg_expand_copy_on_write_datablock(const Depsgraph *depsgraph, const IDNode *id_node) {
	const ID *id_orig = id_node->id_orig;
	ID *id_cow = id_node->id_cow;
	const int id_cow_recalc = id_cow->recalc;

	/* No need to expand such datablocks, their copied ID is same as original
	 * one already. */
	if (!deg_copy_on_write_is_needed(id_orig)) {
		return id_cow;
	}

	/* Sanity checks. */
	ROSE_assert(check_datablock_expanded(id_cow) == false);

	/* Copy data from original ID to a copied version. */
	/* TODO(sergey): Avoid doing full ID copy somehow, make Mesh to reference
	 * original geometry arrays for until those are modified. */
	/* TODO(sergey): We do some trickery with temp bmain and extra ID pointer
	 * just to be able to use existing API. Ideally we need to replace this with
	 * in-place copy from existing datablock to a prepared memory.
	 *
	 * NOTE: We don't use BKE_main_{new,free} because:
	 * - We don't want heap-allocations here.
	 * - We don't want bmain's content to be freed when main is freed. */
	bool done = false;
	/* First we handle special cases which are not covered by BKE_id_copy() yet.
	 * or cases where we want to do something smarter than simple datablock
	 * copy. */
	const ID_Type id_type = GS(id_orig->name);
	switch (id_type) {
		case ID_SCE: {
			done = scene_copy_inplace_no_main((Scene *)id_orig, (Scene *)id_cow);
			if (done) {
				/* NOTE: This is important to do before remap, because this
				 * function will make it so less IDs are to be remapped. */
				scene_setup_view_layers_before_remap(depsgraph, id_node, (Scene *)id_cow);
			}
			break;
		}
		case ID_ME: {
			/* TODO(sergey): Ideally we want to handle meshes in a special
			 * manner here to avoid initial copy of all the geometry arrays. */
			break;
		}
		default:
			break;
	}
	if (!done) {
		done = id_copy_inplace_no_main(id_orig, id_cow);
	}
	if (!done) {
		ROSE_assert_msg(0, "No idea how to perform CoW on datablock");
	}

#ifdef NESTED_ID_NASTY_WORKAROUND
	ntree_hack_remap_pointers(depsgraph, id_cow);
#endif
	/* Do it now, so remapping will understand that possibly remapped self ID
	 * is not to be remapped again. */
	deg_tag_copy_on_write_id(id_cow, id_orig);
	/* Perform remapping of the nodes. */
	RemapCallbackUserData user_data = {nullptr};
	user_data.depsgraph = depsgraph;
	KER_library_foreach_ID_link(nullptr, id_cow, foreach_libblock_remap_callback, (void *)&user_data, 0);
	/* Correct or tweak some pointers which are not taken care by foreach
	 * from above. */
	update_id_after_copy(depsgraph, id_node, id_orig, id_cow);
	id_cow->recalc = id_cow_recalc;
	return id_cow;
}

}

ID *deg_update_copy_on_write_datablock(const Depsgraph *depsgraph, const IDNode *id_node) {
	const ID *id_orig = id_node->id_orig;
	ID *id_cow = id_node->id_cow;
	/* Similar to expansion, no need to do anything here. */
	if (!deg_copy_on_write_is_needed(id_orig)) {
		return id_cow;
	}

	RuntimeBackup backup(depsgraph);
	backup.init_from_id(id_cow);
	deg_free_copy_on_write_datablock(id_cow);
	deg_expand_copy_on_write_datablock(depsgraph, id_node);
	backup.restore_to_id(id_cow);
	return id_cow;
}

/**
 * \note Depsgraph is supposed to have ID node already.
 */
ID *deg_update_copy_on_write_datablock(const Depsgraph *depsgraph, ID *id_orig) {
	IDNode *id_node = depsgraph->find_id_node(id_orig);
	ROSE_assert(id_node != nullptr);
	return deg_update_copy_on_write_datablock(depsgraph, id_node);
}

void deg_evaluate_copy_on_write(::Depsgraph *graph, const IDNode *id_node) {
	const Depsgraph *depsgraph = reinterpret_cast<const Depsgraph *>(graph);
	if (id_node->id_orig == &depsgraph->scene->id) {
		/* NOTE: This is handled by eval_ctx setup routines, which
		 * ensures scene and view layer pointers are valid. */
		return;
	}
	deg_update_copy_on_write_datablock(depsgraph, id_node);
}

void deg_free_copy_on_write_datablock(ID *id_cow) {
	if (!check_datablock_expanded(id_cow)) {
		/* Actual content was never copied on top of CoW block, we have
		 * nothing to free. */
		return;
	}
	const ID_Type type = GS(id_cow->name);
#ifdef NESTED_ID_NASTY_WORKAROUND
	nested_id_hack_discard_pointers(id_cow);
#endif
	switch (type) {
		case ID_OB: {
			/* TODO(sergey): This workaround is only to prevent free derived
			 * caches from modifying object->data. This is currently happening
			 * due to mesh/curve data-block bound-box tagging dirty. */
			Object *ob_cow = (Object *)id_cow;
			ob_cow->data = nullptr;
			break;
		}
		default:
			break;
	}
	KER_libblock_free_datablock(id_cow, 0);
	KER_libblock_free_data(id_cow, false);
	/* Signal datablock as not being expanded. */
	id_cow->name[0] = '\0';
}

void deg_tag_copy_on_write_id(ID *id_cow, const ID *id_orig) {
	ROSE_assert(id_cow != id_orig);
	ROSE_assert((id_orig->tag & ID_TAG_COPIED_ON_WRITE) == 0);
	id_cow->tag |= ID_TAG_COPIED_ON_WRITE;
	id_cow->orig_id = (ID *)id_orig;
}

bool deg_copy_on_write_is_expanded(const ID *id_cow) {
	return check_datablock_expanded(id_cow);
}

bool deg_copy_on_write_is_needed(const ID *id_orig) {
	const ID_Type id_type = GS(id_orig->name);
	return deg_copy_on_write_is_needed(id_type);
}
bool deg_copy_on_write_is_needed(const ID_Type id_type) {
	return ID_TYPE_IS_COW(id_type);
}

}  // namespace rose::depsgraph
