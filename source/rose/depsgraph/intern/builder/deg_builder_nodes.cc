#include "intern/builder/deg_builder_nodes.hh"

#include <cstdio>
#include <cstdlib>

#include "MEM_guardedalloc.h"

#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_action.h"
#include "KER_anim_data.h"
#include "KER_anim_sys.h"
#include "KER_armature.h"
#include "KER_collection.h"
#include "KER_fcurve.h"
#include "KER_idprop.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_mesh.hh"
#include "KER_modifier.h"
#include "KER_object.h"
#include "KER_scene.h"

#include "RNA_access.h"
#include "RNA_prototypes.h"
#include "RNA_types.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"

#include "intern/builder/deg_builder.hh"
#include "intern/depsgraph.hh"
#include "intern/depsgraph_tag.hh"
#include "intern/depsgraph_types.hh"
#include "intern/eval/deg_eval_copy_on_write.hh"
#include "intern/node/deg_node.hh"
#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"

namespace rose::depsgraph {

DepsgraphNodeBuilder::DepsgraphNodeBuilder(Main *bmain, Depsgraph *graph, DepsgraphBuilderCache *cache) : DepsgraphBuilder(bmain, graph, cache), scene_(nullptr), view_layer_(nullptr), view_layer_index_(-1), collection_(nullptr), is_parent_collection_visible_(true) {
}

DepsgraphNodeBuilder::~DepsgraphNodeBuilder() {
	for (IDInfo *id_info : id_info_hash_.values()) {
		if (id_info->id_cow != nullptr) {
			deg_free_copy_on_write_datablock(id_info->id_cow);
			MEM_freeN(id_info->id_cow);
		}
		MEM_freeN(id_info);
	}
}

IDNode *DepsgraphNodeBuilder::add_id_node(ID *id) {
	ROSE_assert(id->uuid != MAIN_ID_SESSION_UUID_UNSET);

	const ID_Type id_type = GS(id->name);
	IDNode *id_node = nullptr;
	ID *id_cow = nullptr;
	IDComponentsMask previously_visible_components_mask = 0;
	uint32_t previous_eval_flags = 0;
	DEGCustomDataMeshMasks previous_customdata_masks;
	IDInfo *id_info = id_info_hash_.lookup_default(id->uuid, nullptr);
	if (id_info != nullptr) {
		id_cow = id_info->id_cow;
		previously_visible_components_mask = id_info->previously_visible_components_mask;
		previous_eval_flags = id_info->previous_eval_flags;
		previous_customdata_masks = id_info->previous_customdata_masks;
		/* Tag ID info to not free the CoW ID pointer. */
		id_info->id_cow = nullptr;
	}
	id_node = graph_->add_id_node(id, id_cow);
	id_node->previously_visible_components_mask = previously_visible_components_mask;
	id_node->previous_eval_flags = previous_eval_flags;
	id_node->previous_customdata_masks = previous_customdata_masks;

	/* NOTE: Zero number of components indicates that ID node was just created. */
	const bool is_newly_created = id_node->components.is_empty();

	if (is_newly_created) {
		if (deg_copy_on_write_is_needed(id_type)) {
			ComponentNode *comp_cow = id_node->add_component(NodeType::COPY_ON_WRITE);
			OperationNode *op_cow = comp_cow->add_operation([id_node](::Depsgraph *depsgraph) {
				deg_evaluate_copy_on_write(depsgraph, id_node);
			}, OperationCode::COPY_ON_WRITE, "", -1);
			graph_->operations.append(op_cow);
		}

		ComponentNode *visibility_component = id_node->add_component(NodeType::VISIBILITY);
		OperationNode *visibility_operation = visibility_component->add_operation(nullptr, OperationCode::OPERATION, "", -1);
		/* Pin the node so that it and its relations are preserved by the unused nodes/relations
		 * deletion. This is mainly to make it easier to debug visibility. */
		visibility_operation->flag |= OperationFlag::DEPSOP_FLAG_PINNED;
		graph_->operations.append(visibility_operation);
	}
	return id_node;
}

IDNode *DepsgraphNodeBuilder::find_id_node(ID *id) {
	return graph_->find_id_node(id);
}

TimeSourceNode *DepsgraphNodeBuilder::add_time_source() {
	return graph_->add_time_source();
}

ComponentNode *DepsgraphNodeBuilder::add_component_node(ID *id, NodeType comp_type, const char *comp_name) {
	IDNode *id_node = add_id_node(id);
	ComponentNode *comp_node = id_node->add_component(comp_type, comp_name);
	comp_node->owner = id_node;
	return comp_node;
}

OperationNode *DepsgraphNodeBuilder::add_operation_node(ComponentNode *comp_node, OperationCode opcode, const DepsEvalOperationCb &op, const char *name, int name_tag) {
	OperationNode *op_node = comp_node->find_operation(opcode, name, name_tag);
	if (op_node == nullptr) {
		op_node = comp_node->add_operation(op, opcode, name, name_tag);
		graph_->operations.append(op_node);
	}
	else {
		fprintf(stderr, "add_operation: Operation already exists - %s has %s at %p\n", comp_node->identifier().c_str(), op_node->identifier().c_str(), op_node);
		ROSE_assert_msg(0, "Should not happen!");
	}
	return op_node;
}

OperationNode *DepsgraphNodeBuilder::add_operation_node(ID *id, NodeType comp_type, const char *comp_name, OperationCode opcode, const DepsEvalOperationCb &op, const char *name, int name_tag) {
	ComponentNode *comp_node = add_component_node(id, comp_type, comp_name);
	return add_operation_node(comp_node, opcode, op, name, name_tag);
}

OperationNode *DepsgraphNodeBuilder::add_operation_node(ID *id, NodeType comp_type, OperationCode opcode, const DepsEvalOperationCb &op, const char *name, int name_tag) {
	return add_operation_node(id, comp_type, "", opcode, op, name, name_tag);
}

OperationNode *DepsgraphNodeBuilder::ensure_operation_node(ID *id, NodeType comp_type, const char *comp_name, OperationCode opcode, const DepsEvalOperationCb &op, const char *name, int name_tag) {
	OperationNode *operation = find_operation_node(id, comp_type, comp_name, opcode, name, name_tag);
	if (operation != nullptr) {
		return operation;
	}
	return add_operation_node(id, comp_type, comp_name, opcode, op, name, name_tag);
}

OperationNode *DepsgraphNodeBuilder::ensure_operation_node(ID *id, NodeType comp_type, OperationCode opcode, const DepsEvalOperationCb &op, const char *name, int name_tag) {
	OperationNode *operation = find_operation_node(id, comp_type, opcode, name, name_tag);
	if (operation != nullptr) {
		return operation;
	}
	return add_operation_node(id, comp_type, opcode, op, name, name_tag);
}

bool DepsgraphNodeBuilder::has_operation_node(ID *id, NodeType comp_type, const char *comp_name, OperationCode opcode, const char *name, int name_tag) {
	return find_operation_node(id, comp_type, comp_name, opcode, name, name_tag) != nullptr;
}

OperationNode *DepsgraphNodeBuilder::find_operation_node(ID *id, NodeType comp_type, const char *comp_name, OperationCode opcode, const char *name, int name_tag) {
	ComponentNode *comp_node = add_component_node(id, comp_type, comp_name);
	return comp_node->find_operation(opcode, name, name_tag);
}

OperationNode *DepsgraphNodeBuilder::find_operation_node(ID *id, NodeType comp_type, OperationCode opcode, const char *name, int name_tag) {
	return find_operation_node(id, comp_type, "", opcode, name, name_tag);
}

ID *DepsgraphNodeBuilder::get_cow_id(const ID *id_orig) const {
	return graph_->get_cow_id(id_orig);
}

ID *DepsgraphNodeBuilder::ensure_cow_id(ID *id_orig) {
	if (id_orig->tag & ID_TAG_COPIED_ON_WRITE) {
		/* ID is already remapped to copy-on-write. */
		return id_orig;
	}
	IDNode *id_node = add_id_node(id_orig);
	return id_node->id_cow;
}

void DepsgraphNodeBuilder::begin_build() {
	/* Store existing copy-on-write versions of datablock, so we can re-use
	 * them for new ID nodes. */
	for (IDNode *id_node : graph_->id_nodes) {
		/* It is possible that the ID does not need to have CoW version in which case id_cow is the
		 * same as id_orig. Additionally, such ID might have been removed, which makes the check
		 * for whether id_cow is expanded to access freed memory. In order to deal with this we
		 * check whether CoW is needed based on a scalar value which does not lead to access of
		 * possibly deleted memory. */
		IDInfo *id_info = (IDInfo *)MEM_mallocN(sizeof(IDInfo), "depsgraph id info");
		if (deg_copy_on_write_is_needed(id_node->id_type) && deg_copy_on_write_is_expanded(id_node->id_cow) && id_node->id_orig != id_node->id_cow) {
			id_info->id_cow = id_node->id_cow;
		}
		else {
			id_info->id_cow = nullptr;
		}
		id_info->previously_visible_components_mask = id_node->visible_components_mask;
		id_info->previous_eval_flags = id_node->eval_flags;
		id_info->previous_customdata_masks = id_node->customdata_masks;
		ROSE_assert(!id_info_hash_.contains(id_node->id_orig_session_uuid));
		id_info_hash_.add_new(id_node->id_orig_session_uuid, id_info);
		id_node->id_cow = nullptr;
	}

	for (OperationNode *op_node : graph_->entry_tags) {
		ComponentNode *comp_node = op_node->owner;
		IDNode *id_node = comp_node->owner;

		SavedEntryTag entry_tag;
		entry_tag.id_orig = id_node->id_orig;
		entry_tag.component_type = comp_node->type;
		entry_tag.opcode = op_node->opcode;
		entry_tag.name = op_node->name;
		entry_tag.name_tag = op_node->name_tag;
		saved_entry_tags_.append(entry_tag);
	}

	/* Make sure graph has no nodes left from previous state. */
	graph_->clear_all_nodes();
	graph_->operations.clear();
	graph_->entry_tags.clear();
}

void DepsgraphNodeBuilder::modifier_walk(void *user_data, struct Object * /*object*/, struct ID **idpoin, int /*cb_flag*/) {
	BuilderWalkUserData *data = (BuilderWalkUserData *)user_data;
	ID *id = *idpoin;
	if (id == nullptr) {
		return;
	}
	switch (GS(id->name)) {
		case ID_OB:
			data->builder->build_object(-1, (Object *)id, DEG_ID_LINKED_INDIRECTLY, false);
			break;
		default:
			data->builder->build_id(id);
			break;
	}
}

/* Util callbacks for `KER_library_foreach_ID_link`, used to detect when a COW ID is using ID
 * pointers that are either:
 *  - COW ID pointers that do not exist anymore in current depsgraph.
 *  - Orig ID pointers that do have now a COW version in current depsgraph.
 * In both cases, it means the COW ID user needs to be flushed, to ensure its pointers are properly
 * remapped.
 *
 * NOTE: This is split in two, a static function and a public method of the node builder, to allow
 * the code to access the builder's data more easily. */
int DepsgraphNodeBuilder::foreach_id_cow_detect_need_for_update_callback(ID *id_cow_self, ID *id_pointer) {
	if (id_pointer->orig_id == nullptr) {
		/* `id_cow_self` uses a non-cow ID, if that ID has a COW copy in current depsgraph its owner
		 * needs to be remapped, i.e. COW-flushed. */
		IDNode *id_node = find_id_node(id_pointer);
		if (id_node != nullptr && id_node->id_cow != nullptr) {
			graph_id_tag_update(main_, graph_, id_cow_self->orig_id, ID_RECALC_COPY_ON_WRITE, DEG_UPDATE_SOURCE_RELATIONS);
			return IDWALK_RET_STOP_ITER;
		}
	}
	else {
		/* `id_cow_self` uses a COW ID, if that COW copy is removed from current depsgraph its owner
		 * needs to be remapped, i.e. COW-flushed. */
		/* NOTE: at that stage, old existing COW copies that are to be removed from current state of
		 * evaluated depsgraph are still valid pointers, they are freed later (typically during
		 * destruction of the builder itself). */
		IDNode *id_node = find_id_node(id_pointer->orig_id);
		if (id_node == nullptr) {
			graph_id_tag_update(main_, graph_, id_cow_self->orig_id, ID_RECALC_COPY_ON_WRITE, DEG_UPDATE_SOURCE_RELATIONS);
			return IDWALK_RET_STOP_ITER;
		}
	}
	return IDWALK_RET_NOP;
}

static int foreach_id_cow_detect_need_for_update_callback(LibraryIDLinkCallbackData *cb_data) {
	ID *id = *cb_data->self_ptr;
	if (id == nullptr) {
		return IDWALK_RET_NOP;
	}

	DepsgraphNodeBuilder *builder = static_cast<DepsgraphNodeBuilder *>(cb_data->user_data);
	ID *id_cow_self = cb_data->self_id;

	return builder->foreach_id_cow_detect_need_for_update_callback(id_cow_self, id);
}

void DepsgraphNodeBuilder::update_invalid_cow_pointers() {
	/* NOTE: Currently the only ID types that depsgraph may decide to not evaluate/generate COW
	 * copies for, even though they are referenced by other data-blocks, are Collections and Objects
	 * (through their various visibility flags, and the ones from #LayerCollections too). However,
	 * this code is kept generic as it makes it more future-proof, and optimization here would give
	 * negligible performance improvements in typical cases.
	 *
	 * NOTE: This mechanism may also 'fix' some missing update tagging from non-depsgraph code in
	 * some cases. This is slightly unfortunate (as it may hide issues in other parts of Blender
	 * code), but cannot really be avoided currently. */

	for (const IDNode *id_node : graph_->id_nodes) {
		if (id_node->previously_visible_components_mask == 0) {
			/* Newly added node/ID, no need to check it. */
			continue;
		}
		if (ELEM(id_node->id_cow, id_node->id_orig, nullptr)) {
			/* Node/ID with no COW data, no need to check it. */
			continue;
		}
		if ((id_node->id_cow->recalc & ID_RECALC_COPY_ON_WRITE) != 0) {
			/* Node/ID already tagged for COW flush, no need to check it. */
			continue;
		}
		KER_library_foreach_ID_link(nullptr, id_node->id_cow, rose::depsgraph::foreach_id_cow_detect_need_for_update_callback, this, IDWALK_READONLY);
	}
}

void DepsgraphNodeBuilder::tag_previously_tagged_nodes() {
	for (const SavedEntryTag &entry_tag : saved_entry_tags_) {
		IDNode *id_node = find_id_node(entry_tag.id_orig);
		if (id_node == nullptr) {
			continue;
		}
		ComponentNode *comp_node = id_node->find_component(entry_tag.component_type);
		if (comp_node == nullptr) {
			continue;
		}
		OperationNode *op_node = comp_node->find_operation(entry_tag.opcode, entry_tag.name.c_str(), entry_tag.name_tag);
		if (op_node == nullptr) {
			continue;
		}
		/* Since the tag is coming from a saved copy of entry tags, this means
		 * that originally node was explicitly tagged for user update. */
		op_node->tag_update(graph_, DEG_UPDATE_SOURCE_USER_EDIT);
	}
}

void DepsgraphNodeBuilder::end_build() {
	tag_previously_tagged_nodes();
	update_invalid_cow_pointers();
}

void DepsgraphNodeBuilder::build_id(ID *id) {
	if (id == nullptr) {
		return;
	}

	const ID_Type idtype = GS(id->name);
	switch (idtype) {
		case ID_AC: {
			build_action(reinterpret_cast<Action *>(id));
		} break;
		case ID_AR: {
			build_armature(reinterpret_cast<Armature *>(id));
		} break;
		case ID_CA: {
			// TODO!
		} break;
		case ID_GR: {
			build_collection(nullptr, reinterpret_cast<Collection *>(id));
		} break;
		case ID_OB: {
			build_object(-1, reinterpret_cast<Object *>(id), DEG_ID_LINKED_INDIRECTLY, false);
		} break;
		case ID_ME: {
			build_object_data_geometry_datablock(id);
		} break;
		case ID_SCE: {
			build_scene_parameters(reinterpret_cast<Scene *>(id));
		} break;

		default: {
			ROSE_assert(!deg_copy_on_write_is_needed(idtype));
			build_generic_id(id);
		} break;
	}
}

void DepsgraphNodeBuilder::build_generic_id(ID *id) {
	if (built_map_.checkIsBuiltAndTag(id)) {
		return;
	}

	build_idproperties(id->properties);
	build_animdata(id);
	build_parameters(id);
}

static void build_idproperties_callback(IDProperty *id_property, void *user_data) {
	DepsgraphNodeBuilder *builder = reinterpret_cast<DepsgraphNodeBuilder *>(user_data);
	ROSE_assert(id_property->type == IDP_ID);
	builder->build_id(reinterpret_cast<ID *>(id_property->data.pointer));
}

void DepsgraphNodeBuilder::build_idproperties(IDProperty *id_property) {
	IDP_foreach_property(id_property, IDP_TYPE_FILTER_ID, build_idproperties_callback, this);
}

void DepsgraphNodeBuilder::build_animdata(ID *id) {
	/* Regular animation. */
	AnimData *adt = KER_animdata_from_id(id);
	if (adt == nullptr) {
		return;
	}
	if (adt->action != nullptr) {
		build_action(adt->action);
	}
	/* Make sure ID node exists. */
	(void)add_id_node(id);
	ID *id_cow = get_cow_id(id);
	if (adt->action != NULL) {
		OperationNode *operation_node;
		/* Explicit entry operation. */
		operation_node = add_operation_node(id, NodeType::ANIMATION, OperationCode::ANIMATION_ENTRY);
		operation_node->set_as_entry();
		/* All the evaluation nodes. */
		add_operation_node(id, NodeType::ANIMATION, OperationCode::ANIMATION_EVAL, [id_cow](::Depsgraph *depsgraph) {
			KER_animsys_eval_animdata(depsgraph, id_cow);
		});
		/* Explicit exit operation. */
		operation_node = add_operation_node(id, NodeType::ANIMATION, OperationCode::ANIMATION_EXIT);
		operation_node->set_as_exit();
	}
}

void DepsgraphNodeBuilder::build_parameters(ID *id) {
	(void)add_id_node(id);
	OperationNode *op_node;

	/* Explicit entry. */
	op_node = add_operation_node(id, NodeType::PARAMETERS, OperationCode::PARAMETERS_ENTRY);
	op_node->set_as_entry();
	/* Generic evaluation node. */

	if (ID_TYPE_SUPPORTS_PARAMS_WITHOUT_COW(GS(id->name))) {
		ID *id_cow = get_cow_id(id);
		add_operation_node(id, NodeType::PARAMETERS, OperationCode::PARAMETERS_EVAL, [id_cow, id](::Depsgraph * /*depsgraph*/) {
			KER_id_eval_properties_copy(id_cow, id);
		});
	}
	else {
		add_operation_node(id, NodeType::PARAMETERS, OperationCode::PARAMETERS_EVAL);
	}

	/* Explicit exit operation. */
	op_node = add_operation_node(id, NodeType::PARAMETERS, OperationCode::PARAMETERS_EXIT);
	op_node->set_as_exit();
}

void DepsgraphNodeBuilder::build_action(Action *action) {
	if (built_map_.checkIsBuiltAndTag(action)) {
		return;
	}
	build_idproperties(action->id.properties);
	add_operation_node(&action->id, NodeType::ANIMATION, OperationCode::ANIMATION_EVAL);
}

void DepsgraphNodeBuilder::build_armature(Armature *armature) {
	if (built_map_.checkIsBuiltAndTag(armature)) {
		return;
	}
	build_idproperties(armature->id.properties);
	build_animdata(&armature->id);
	build_parameters(&armature->id);
	/* Make sure pose is up-to-date with armature updates. */
	Armature *armature_cow = (Armature *)get_cow_id(&armature->id);
	add_operation_node(&armature->id, NodeType::ARMATURE, OperationCode::ARMATURE_EVAL, [armature_cow](::Depsgraph *depsgraph) {
	 	// KER_armature_refresh_layer_used(depsgraph, armature_cow);
	});
	build_armature_bones(&armature->bonebase);
}

void DepsgraphNodeBuilder::build_armature_bones(ListBase *bones) {
	LISTBASE_FOREACH(Bone *, bone, bones) {
		build_idproperties(bone->prop);
		build_armature_bones(&bone->childbase);
	}
}

void DepsgraphNodeBuilder::build_collection(LayerCollection *from_layer_collection, Collection *collection) {
	const bool is_collection_restricted = (collection->flag & COLLECTION_HIDE_VIEWPORT);
	const bool is_collection_visible = !is_collection_restricted && is_parent_collection_visible_;
	IDNode *id_node;
	if (built_map_.checkIsBuiltAndTag(collection)) {
		id_node = find_id_node(&collection->id);
		if (is_collection_visible && id_node->is_directly_visible == false && id_node->is_collection_fully_expanded == true) {
			/* Collection became visible, make sure nested collections and
			 * objects are poked with the new visibility flag, since they
			 * might become visible too. */
		}
		else if (from_layer_collection == nullptr && !id_node->is_collection_fully_expanded) {
			/* Initially collection was built from layer now, and was requested
			 * to not recurse into object. But now it's asked to recurse into all objects. */
		}
		else {
			return;
		}
	}
	else {
		/* Collection itself. */
		id_node = add_id_node(&collection->id);
		id_node->is_directly_visible = is_collection_visible;

		build_idproperties(collection->id.properties);
		add_operation_node(&collection->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL_DONE);
	}
	if (from_layer_collection != nullptr) {
		/* If we came from layer collection we don't go deeper, view layer
		 * builder takes care of going deeper. */
		return;
	}
	/* Backup state. */
	Collection *current_state_collection = collection_;
	const bool is_current_parent_collection_visible = is_parent_collection_visible_;
	/* Modify state as we've entered new collection/ */
	collection_ = collection;
	is_parent_collection_visible_ = is_collection_visible;
	/* Build collection objects. */
	LISTBASE_FOREACH(CollectionObject *, cob, &collection->objects) {
		build_object(-1, cob->object, DEG_ID_LINKED_INDIRECTLY, is_collection_visible);
	}
	/* Build child collections. */
	LISTBASE_FOREACH(CollectionChild *, child, &collection->children) {
		build_collection(nullptr, child->collection);
	}
	/* Restore state. */
	collection_ = current_state_collection;
	is_parent_collection_visible_ = is_current_parent_collection_visible;
	id_node->is_collection_fully_expanded = true;
}

void DepsgraphNodeBuilder::build_object(int base_index, Object *object, eDepsNode_LinkedState_Type linked_state, bool is_visible) {
	const bool has_object = built_map_.checkIsBuiltAndTag(object);

	/* When there is already object in the dependency graph accumulate visibility an linked state
	 * flags. Only do it on the object itself (apart from very special cases) and leave dealing with
	 * visibility of dependencies to the visibility flush step which happens at the end of the build
	 * process. */
	if (has_object) {
		IDNode *id_node = find_id_node(&object->id);
		if (id_node->linked_state == DEG_ID_LINKED_INDIRECTLY) {
			build_object_flags(base_index, object, linked_state);
		}
		id_node->linked_state = ROSE_MAX(id_node->linked_state, linked_state);
		id_node->is_directly_visible |= is_visible;
		id_node->has_base |= (base_index != -1);

		/* There is no relation path which will connect current object with all the ones which come
		 * via the instanced collection, so build the collection again. Note that it will do check
		 * whether visibility update is needed on its own. */
		// build_object_instance_collection(object, is_visible);
		return;
	}

	/* Create ID node for object and begin init. */
	IDNode *id_node = add_id_node(&object->id);
	Object *object_cow = get_cow_datablock(object);
	id_node->linked_state = linked_state;
	/* NOTE: Scene is nullptr when building dependency graph for render pipeline.
	 * Probably need to assign that to something non-nullptr, but then the logic here will still be
	 * somewhat weird. */
	if (scene_ != nullptr && object == scene_->camera) {
		id_node->is_directly_visible = true;
	}
	else {
		id_node->is_directly_visible = is_visible;
	}
	id_node->has_base |= (base_index != -1);
	/* Various flags, flushing from bases/collections. */
	build_object_from_layer(base_index, object, linked_state);
	/* Transform. */
	build_object_transform(object);
	/* Parent. */
	if (object->parent != nullptr) {
		build_object(-1, object->parent, DEG_ID_LINKED_INDIRECTLY, is_visible);
	}
	/* Modifiers. */
	if (object->modifiers.first != nullptr) {
		BuilderWalkUserData data;
		data.builder = this;
		KER_modifiers_foreach_ID_link(object, modifier_walk, &data);
	}
	/* Object data. */
	build_object_data(object);
	/* Parameters, used by both drivers/animation and also to inform dependency
	 * from object's data. */
	build_parameters(&object->id);
	build_idproperties(object->id.properties);
	/* Build animation data,
	 *
	 * Do it now because it's possible object data will affect
	 * on object's level animation, for example in case of rebuilding
	 * pose for proxy. */
	build_animdata(&object->id);
	/* Synchronization back to original object. */
	add_operation_node(&object->id, NodeType::SYNCHRONIZATION, OperationCode::SYNCHRONIZE_TO_ORIGINAL, [object_cow](::Depsgraph *depsgraph) {
		KER_object_sync_to_original(depsgraph, object_cow);
	});
}

void DepsgraphNodeBuilder::build_object_data(Object *object) {
	if (object->data == nullptr) {
		return;
	}

	switch (object->type) {
		case OB_MESH: {
			build_object_data_geometry(object);
		} break;
		case OB_ARMATURE: {
			build_object_data_armature(object);
		} break;
		case OB_CAMERA: {
			// TODO!
		} break;
		default: {
			ID *obdata = (ID *)object->data;
			if (!built_map_.checkIsBuilt(obdata)) {
				build_animdata(obdata);
			}
		} break;
	}
}

void DepsgraphNodeBuilder::build_object_data_armature(Object *object) {
	Armature *armature = (Armature *)object->data;
	Scene *scene_cow = get_cow_datablock(scene_);
	Object *object_cow = get_cow_datablock(object);
	OperationNode *op_node;

	/**
	 * Animation and/or drivers linking pose-bones to base-armature used to define them.
	 *
	 * NOTE: AnimData here is really used to control animated deform properties,
	 *       which ideally should be able to be unique across different
	 *       instances. Eventually, we need some type of proxy/isolation
	 *       mechanism in-between here to ensure that we can use same rig
	 *       multiple times in same scene.
	 */
	/* Armature. */
	build_armature(armature);
	/* Rebuild pose if not up to date. */
	if (object->pose == nullptr || (object->pose->flag & POSE_RECALC)) {
		/* By definition, no need to tag depsgraph as dirty from here, so we can pass nullptr bmain. */
		KER_pose_rebuild(nullptr, object, armature, true);
	}
	/* Speed optimization for animation lookups. */
	if (object->pose != nullptr) {
		KER_pose_channels_hash_ensure(object->pose);
	}
	/**
	 * Pose Rig Graph
	 * ==============
	 *
	 * Pose Component:
	 * - Mainly used for referencing Bone components.
	 * - This is where the evaluation operations for init/exec/cleanup
	 *   (ik) solvers live, and are later hooked up (so that they can be
	 *   interleaved during runtime) with bone-operations they depend on/affect.
	 * - init_pose_eval() and cleanup_pose_eval() are absolute first and last
	 *   steps of pose eval process. ALL bone operations must be performed
	 *   between these two...
	 *
	 * Bone Component:
	 * - Used for representing each bone within the rig
	 * - Acts to encapsulate the evaluation operations (base matrix + parenting,
	 *   and constraint stack) so that they can be easily found.
	 * - Everything else which depends on bone-results hook up to the component
	 *   only so that we can redirect those to point at either the
	 *   post-IK/post-constraint/post-matrix steps, as needed. */
	/* Pose eval context. */
	op_node = add_operation_node(&object->id, NodeType::EVAL_POSE, OperationCode::POSE_INIT, [scene_cow, object_cow](::Depsgraph *depsgraph) {
		KER_pose_eval_init(depsgraph, scene_cow, object_cow);
	});
	op_node->set_as_entry();
	op_node = add_operation_node(&object->id, NodeType::EVAL_POSE, OperationCode::POSE_INIT_IK, [scene_cow, object_cow](::Depsgraph *depsgraph) {
		KER_pose_eval_init_ik(depsgraph, scene_cow, object_cow);
	});
	add_operation_node(&object->id, NodeType::EVAL_POSE, OperationCode::POSE_CLEANUP, [scene_cow, object_cow](::Depsgraph *depsgraph) {
		KER_pose_eval_cleanup(depsgraph, scene_cow, object_cow);
	});
	op_node = add_operation_node(&object->id, NodeType::EVAL_POSE, OperationCode::POSE_DONE, [object_cow](::Depsgraph *depsgraph) {
		KER_pose_eval_done(depsgraph, object_cow);
	});
	op_node->set_as_exit();

	/* Bones. */
	size_t index;
	LISTBASE_FOREACH_INDEX(PoseChannel *, pchan, &object->pose->channelbase, index) {
		/* Node for bone evaluation. */
		op_node = add_operation_node(&object->id, NodeType::BONE, pchan->name, OperationCode::BONE_LOCAL);
		op_node->set_as_entry();

		add_operation_node(&object->id, NodeType::BONE, pchan->name, OperationCode::BONE_POSE_PARENT, [scene_cow, object_cow, index](::Depsgraph *depsgraph) {
			KER_pose_eval_bone(depsgraph, scene_cow, object_cow, index);
		});

		/* NOTE: Dedicated noop for easier relationship construction. */
		add_operation_node(&object->id, NodeType::BONE, pchan->name, OperationCode::BONE_READY);

		op_node = add_operation_node(&object->id, NodeType::BONE, pchan->name, OperationCode::BONE_DONE, [object_cow, index](::Depsgraph *depsgraph) {
			KER_pose_bone_done(depsgraph, object_cow, index);
		});

		op_node->set_as_exit();
	}
}

void DepsgraphNodeBuilder::build_object_data_geometry(Object *object) {
	OperationNode *op_node;
	Scene *scene_cow = get_cow_datablock(scene_);
	Object *object_cow = get_cow_datablock(object);
	/* Entry operation, takes care of initialization, and some other
	 * relations which needs to be run prior actual geometry evaluation. */
	op_node = add_operation_node(&object->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL_INIT);
	op_node->set_as_entry();
	/* Geometry evaluation. */
	op_node = add_operation_node(&object->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL, [scene_cow, object_cow](::Depsgraph *depsgraph) {
		KER_object_eval_uber_data(depsgraph, scene_cow, object_cow);
	});
	op_node->set_as_exit();
	/* Geometry. */
	build_object_data_geometry_datablock((ID *)object->data);
}

void DepsgraphNodeBuilder::build_object_data_geometry_datablock(ID *obdata) {
	if (built_map_.checkIsBuiltAndTag(obdata)) {
		return;
	}
	OperationNode *op_node;
	/* Make sure we've got an ID node before requesting CoW pointer. */
	(void)add_id_node((ID *)obdata);
	ID *obdata_cow = get_cow_id(obdata);
	build_idproperties(obdata->properties);
	/* Animation. */
	build_animdata(obdata);

	const ID_Type id_type = GS(obdata->name);
	switch (id_type) {
		case ID_ME: {
			op_node = add_operation_node(obdata, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL, [obdata_cow](::Depsgraph *depsgraph) {
				KER_mesh_eval_geometry(depsgraph, (Mesh *)obdata_cow);
			});
			op_node->set_as_entry();
			break;
		}
	}

	op_node = add_operation_node(obdata, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL_DONE);
	op_node->set_as_exit();
	/* Parameters for driver sources. */
	build_parameters(obdata);
}

void DepsgraphNodeBuilder::build_object_flags(int base_index, Object *object, eDepsNode_LinkedState_Type linked_state) {
	if (base_index == -1) {
		return;
	}
	Scene *scene_cow = get_cow_datablock(scene_);
	Object *object_cow = get_cow_datablock(object);
	const bool is_from_set = (linked_state == DEG_ID_LINKED_VIA_SET);
	/* TODO(sergey): Is this really best component to be used? */
	add_operation_node(&object->id, NodeType::OBJECT_FROM_LAYER, OperationCode::OBJECT_BASE_FLAGS, [view_layer_index = view_layer_index_, scene_cow, object_cow, base_index, is_from_set](::Depsgraph *depsgraph) {
		KER_object_eval_eval_base_flags(depsgraph, scene_cow, view_layer_index, object_cow, base_index, is_from_set);
	});
}

void DepsgraphNodeBuilder::build_object_from_layer(int base_index, Object *object, eDepsNode_LinkedState_Type linked_state) {
	OperationNode *entry_node = add_operation_node(&object->id, NodeType::OBJECT_FROM_LAYER, OperationCode::OBJECT_FROM_LAYER_ENTRY);
	entry_node->set_as_entry();
	OperationNode *exit_node = add_operation_node(&object->id, NodeType::OBJECT_FROM_LAYER, OperationCode::OBJECT_FROM_LAYER_EXIT);
	exit_node->set_as_exit();

	build_object_flags(base_index, object, linked_state);
}

void DepsgraphNodeBuilder::build_object_transform(Object *object) {
	OperationNode *op_node;
	Object *ob_cow = get_cow_datablock(object);
	/* Transform entry operation. */
	op_node = add_operation_node(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_INIT);
	op_node->set_as_entry();
	/* Local transforms (from transform channels - loc/rot/scale + deltas). */
	add_operation_node(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_LOCAL, [ob_cow](::Depsgraph *depsgraph) {
		KER_object_eval_local_transform(depsgraph, ob_cow);
	});
	/* Object parent. */
	if (object->parent != nullptr) {
		add_operation_node(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_PARENT, [ob_cow](::Depsgraph *depsgraph) {
			KER_object_eval_parent(depsgraph, ob_cow);
		});
	}
	/* Rest of transformation update. */
	add_operation_node(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_EVAL, [ob_cow](::Depsgraph *depsgraph) {
		// KER_object_eval_uber_transform(depsgraph, ob_cow);
	});
	/* Operation to take of rigid body simulation. soft bodies and other friends
	 * in the context of point cache invalidation. */
	add_operation_node(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_SIMULATION_INIT);
	/* Object transform is done. */
	op_node = add_operation_node(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_FINAL, [ob_cow](::Depsgraph *depsgraph) {
		KER_object_eval_transform_final(depsgraph, ob_cow);
	});
	op_node->set_as_exit();
}

void DepsgraphNodeBuilder::build_view_layer(Scene *scene, ViewLayer *view_layer, eDepsNode_LinkedState_Type linked_state) {
	view_layer_index_ = 0;
	/* Scene ID block. */
	IDNode *id_node = add_id_node(&scene->id);
	id_node->linked_state = linked_state;
	/* Time source. */
	add_time_source();
	/* Setup currently building context. */
	scene_ = scene;
	view_layer_ = view_layer;
	/* Get pointer to a CoW version of scene ID. */
	Scene *scene_cow = get_cow_datablock(scene);
	/* Scene objects. */
	/* NOTE: Base is used for function bindings as-is, so need to pass CoW base,
	 * but object is expected to be an original one. Hence we go into some
	 * tricks here iterating over the view layer. */
	int base_index = 0;
	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		/* object itself */
		if (need_pull_base_into_graph(base)) {
			/* NOTE: We consider object visible even if it's currently
			 * restricted by the base/restriction flags. Otherwise its drivers
			 * will never be evaluated.
			 *
			 * TODO(sergey): Need to go more granular on visibility checks. */
			build_object(base_index, base->object, linked_state, true);
			base_index++;
		}
	}
	build_layer_collections(&view_layer->layer_collections);
	if (scene->camera != nullptr) {
		build_object(-1, scene->camera, DEG_ID_LINKED_INDIRECTLY, true);
	}
	/* Scene's animation and drivers. */
	if (scene->adt != nullptr) {
		build_animdata(&scene->id);
	}
	/* Collections. */
	add_operation_node(&scene->id, NodeType::LAYER_COLLECTIONS, OperationCode::VIEW_LAYER_EVAL, [view_layer_index = view_layer_index_, scene_cow](::Depsgraph *depsgraph) {
		KER_layer_eval_view_layer_indexed(depsgraph, scene_cow, view_layer_index);
	});
	/* Parameters evaluation for scene relations mainly. */
	build_scene_parameters(scene);
}

void DepsgraphNodeBuilder::build_layer_collections(ListBase *lb) {
	for (LayerCollection *lc = (LayerCollection *)lb->first; lc; lc = lc->next) {
		if (lc->collection->flag & COLLECTION_HIDE_VIEWPORT) {
			continue;
		}
		if ((lc->flag & LAYER_COLLECTION_EXCLUDE) == 0) {
			build_collection(lc, lc->collection);
		}
		build_layer_collections(&lc->layer_collections);
	}
}

void DepsgraphNodeBuilder::build_scene_parameters(Scene *scene) {
	if (built_map_.checkIsBuiltAndTag(scene, BuilderMap::TAG_PARAMETERS)) {
		return;
	}
	build_parameters(&scene->id);
	build_idproperties(scene->id.properties);
	add_operation_node(&scene->id, NodeType::PARAMETERS, OperationCode::SCENE_EVAL);
}

}  // namespace rose::depsgraph
