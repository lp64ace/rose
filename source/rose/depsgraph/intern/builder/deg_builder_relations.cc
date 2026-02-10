#include "intern/builder/deg_builder_relations.hh"

#include <cstdio>
#include <cstdlib>
#include <cstring> /* required for STREQ later on. */

#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_anim_data.h"
#include "KER_anim_sys.h"
#include "KER_armature.h"
#include "KER_collection.h"
#include "KER_idprop.h"
#include "KER_mesh.hh"
#include "KER_modifier.h"
#include "KER_object.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"

#include "RNA_access.h"
#include "RNA_prototypes.h"
#include "RNA_types.h"

#include "intern/builder/deg_builder.hh"
#include "intern/depsgraph_tag.hh"
#include "intern/eval/deg_eval_copy_on_write.hh"

#include "intern/node/deg_node.hh"
#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"
#include "intern/node/deg_node_time.hh"

#include "intern/depsgraph.hh"
#include "intern/depsgraph_relation.hh"
#include "intern/depsgraph_types.hh"

namespace rose::depsgraph {

/* ***************** */
/* Relations Builder */

namespace {

bool check_id_has_anim_component(ID *id) {
	AnimData *adt = KER_animdata_from_id(id);
	if (adt == nullptr) {
		return false;
	}
	return (adt->action != nullptr);
}

bool object_have_geometry_component(const Object *object) {
	return ELEM(object->type, OB_MESH);
}

}  // namespace

DepsgraphRelationBuilder::DepsgraphRelationBuilder(Main *main, Depsgraph *graph, DepsgraphBuilderCache *cache) : DepsgraphBuilder(main, graph, cache), scene_(nullptr), rna_node_query_(graph, this) {
}

void DepsgraphRelationBuilder::begin_build() {
}

void DepsgraphRelationBuilder::build_copy_on_write_relations() {
	for (IDNode *id_node : graph_->id_nodes) {
		build_copy_on_write_relations(id_node);
	}
}

void DepsgraphRelationBuilder::build_copy_on_write_relations(IDNode *id_node) {
	ID *id_orig = id_node->id_orig;

	const ID_Type id_type = GS(id_orig->name);

	if (!deg_copy_on_write_is_needed(id_type)) {
		return;
	}

	TimeSourceKey time_source_key;
	OperationKey copy_on_write_key(id_orig, NodeType::COPY_ON_WRITE, OperationCode::COPY_ON_WRITE);
	/* XXX: This is a quick hack to make Alt-A to work. */
	// add_relation(time_source_key, copy_on_write_key, "Fluxgate capacitor hack");
	/* Resat of code is using rather low level trickery, so need to get some
	 * explicit pointers. */
	Node *node_cow = find_node(copy_on_write_key);
	OperationNode *op_cow = node_cow->get_exit_operation();
	/* Plug any other components to this one. */
	for (ComponentNode *comp_node : id_node->components.values()) {
		if (comp_node->type == NodeType::COPY_ON_WRITE) {
			/* Copy-on-write component never depends on itself. */
			continue;
		}
		if (!comp_node->depends_on_cow()) {
			/* Component explicitly requests to not add relation. */
			continue;
		}
		RelationFlag rel_flag = (RELATION_FLAG_NO_FLUSH | RELATION_FLAG_GODMODE);
		if ((ELEM(id_type, ID_ME) && comp_node->type == NodeType::GEOMETRY)) {
			rel_flag &= ~RELATION_FLAG_NO_FLUSH;
		}
		/* Notes on exceptions:
		 * - Parameters component is where drivers are living. Changing any
		 *   of the (custom) properties in the original datablock (even the
		 *   ones which do not imply other component update) need to make
		 *   sure drivers are properly updated.
		 *   This way, for example, changing ID property will properly poke
		 *   all drivers to be updated.
		 *
		 * - View layers have cached array of bases in them, which is not
		 *   copied by copy-on-write, and not preserved. PROBABLY it is better
		 *   to preserve that cache in copy-on-write, but for the time being
		 *   we allow flush to layer collections component which will ensure
		 *   that cached array of bases exists and is up-to-date. */
		if (ELEM(comp_node->type, NodeType::PARAMETERS, NodeType::LAYER_COLLECTIONS)) {
			rel_flag &= ~RELATION_FLAG_NO_FLUSH;
		}
		/* All entry operations of each component should wait for a proper
		 * copy of ID. */
		OperationNode *op_entry = comp_node->get_entry_operation();
		if (op_entry != nullptr) {
			Relation *rel = graph_->add_new_relation(op_cow, op_entry, "CoW Dependency");
			rel->flag |= rel_flag;
		}
		/* All dangling operations should also be executed after copy-on-write. */
		for (OperationNode *op_node : comp_node->operations_map->values()) {
			if (op_node == op_entry) {
				continue;
			}
			if (op_node->inlinks.is_empty()) {
				Relation *rel = graph_->add_new_relation(op_cow, op_node, "CoW Dependency");
				rel->flag |= rel_flag;
			}
			else {
				bool has_same_comp_dependency = false;
				for (Relation *rel_current : op_node->inlinks) {
					if (rel_current->from->type != NodeType::OPERATION) {
						continue;
					}
					OperationNode *op_node_from = (OperationNode *)rel_current->from;
					if (op_node_from->owner == op_node->owner) {
						has_same_comp_dependency = true;
						break;
					}
				}
				if (!has_same_comp_dependency) {
					Relation *rel = graph_->add_new_relation(op_cow, op_node, "CoW Dependency");
					rel->flag |= rel_flag;
				}
			}
		}
		/* NOTE: We currently ignore implicit relations to an external
		 * data-blocks for copy-on-write operations. This means, for example,
		 * copy-on-write component of Object will not wait for copy-on-write
		 * component of its Mesh. This is because pointers are all known
		 * already so remapping will happen all correct. And then If some object
		 * evaluation step needs geometry, it will have transitive dependency
		 * to Mesh copy-on-write already. */
	}
	/* TODO(sergey): This solves crash for now, but causes too many
	 * updates potentially. */
	if (GS(id_orig->name) == ID_OB) {
		Object *object = (Object *)id_orig;
		ID *object_data_id = (ID *)object->data;
		if (object_data_id != nullptr) {
			if (deg_copy_on_write_is_needed(object_data_id)) {
				OperationKey data_copy_on_write_key(object_data_id, NodeType::COPY_ON_WRITE, OperationCode::COPY_ON_WRITE);
				add_relation(data_copy_on_write_key, copy_on_write_key, "Eval Order", RELATION_FLAG_GODMODE);
			}
		}
		else {
			ROSE_assert(object->type == OB_EMPTY);
		}
	}
}

void DepsgraphRelationBuilder::build_id(ID *id) {
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
			// TODO;
		} break;
		case ID_GR: {
			build_collection(nullptr, nullptr, reinterpret_cast<Collection *>(id));
		} break;
		case ID_OB: {
			build_object(reinterpret_cast<Object *>(id));
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

void DepsgraphRelationBuilder::build_generic_id(ID *id) {
	if (built_map_.checkIsBuiltAndTag(id)) {
		return;
	}

	build_idproperties(id->properties);
	build_animdata(id);
	build_parameters(id);
}

static void build_idproperties_callback(IDProperty *id_property, void *user_data) {
	DepsgraphRelationBuilder *builder = reinterpret_cast<DepsgraphRelationBuilder *>(user_data);
	ROSE_assert(id_property->type == IDP_ID);
	builder->build_id(reinterpret_cast<ID *>(id_property->data.pointer));
}

void DepsgraphRelationBuilder::build_idproperties(IDProperty *id_property) {
	IDP_foreach_property(id_property, IDP_TYPE_FILTER_ID, build_idproperties_callback, this);
}

void DepsgraphRelationBuilder::build_animdata_curves(ID *id) {
	AnimData *adt = KER_animdata_from_id(id);
	if (adt == nullptr) {
		return;
	}
	if (adt->action != nullptr) {
		build_action(adt->action);
	}
	if (adt->action == nullptr) {
		return;
	}

	/* Ensure evaluation order from entry to exit. */
	OperationKey animation_entry_key(id, NodeType::ANIMATION, OperationCode::ANIMATION_ENTRY);
	OperationKey animation_eval_key(id, NodeType::ANIMATION, OperationCode::ANIMATION_EVAL);
	OperationKey animation_exit_key(id, NodeType::ANIMATION, OperationCode::ANIMATION_EXIT);
	add_relation(animation_entry_key, animation_eval_key, "Init -> Eval");
	add_relation(animation_eval_key, animation_exit_key, "Eval -> Exit");
	/* Wire up dependency from action. */
	ComponentKey adt_key(id, NodeType::ANIMATION);
	/* Relation from action itself. */
	if (adt->action != nullptr) {
		ComponentKey action_key(&adt->action->id, NodeType::ANIMATION);
		add_relation(action_key, adt_key, "Action -> Animation");
	}
	/* Get source operations. */
	Node *node_from = get_node(adt_key);
	ROSE_assert(node_from != nullptr);
	if (node_from == nullptr) {
		return;
	}
	OperationNode *operation_from = node_from->get_exit_operation();
	ROSE_assert(operation_from != nullptr);
	/* Build relations from animation operation to properties it changes. */
	if (adt->action != nullptr) {
		ActionChannelBag *bag;
		if ((bag = KER_action_channelbag_for_action_slot_ex(adt->action, adt->handle))) {
			build_animdata_curves_targets(id, adt_key, operation_from, bag->fcurves, bag->totcurve);
		}
	}
}
void DepsgraphRelationBuilder::build_animdata_curves_targets(ID *id, ComponentKey &adt_key, OperationNode *operation_from, FCurve **curves, int totcurve) {
	/* Iterate over all curves and build relations. */
	PointerRNA id_ptr = RNA_id_pointer_create(id);
	for (int curve = 0; curve < totcurve; curve++) {
		FCurve *fcu = curves[curve];

		PathResolvedRNA resolved;
		if (!KER_animsys_rna_curve_resolve(&id_ptr, fcu, &resolved)) {
			continue;
		}

		Node *node_to = rna_node_query_.find_node(&resolved.ptr, resolved.property, RNAPointerSource::ENTRY);
		if (node_to == nullptr) {
			continue;
		}
		OperationNode *operation_to = node_to->get_entry_operation();
		/* NOTE: Special case for bones, avoid relation from animation to
		 * each of the bones. Bone evaluation could only start from pose
		 * init anyway. */
		if (operation_to->opcode == OperationCode::BONE_LOCAL) {
			OperationKey pose_init_key(id, NodeType::EVAL_POSE, OperationCode::POSE_INIT);
			add_relation(adt_key, pose_init_key, "Animation -> Prop", RELATION_CHECK_BEFORE_ADD);
			continue;
		}
		graph_->add_new_relation(operation_from, operation_to, "Animation -> Prop", RELATION_CHECK_BEFORE_ADD);
		/* It is possible that animation is writing to a nested ID data-block,
		 * need to make sure animation is evaluated after target ID is copied. */
		const IDNode *id_node_from = operation_from->owner->owner;
		const IDNode *id_node_to = operation_to->owner->owner;
		if (id_node_from != id_node_to) {
			ComponentKey cow_key(id_node_to->id_orig, NodeType::COPY_ON_WRITE);
			add_relation(cow_key, adt_key, "Animated CoW -> Animation", RELATION_CHECK_BEFORE_ADD | RELATION_FLAG_NO_FLUSH);
		}
	}
}

void DepsgraphRelationBuilder::build_animdata(ID *id) {
	build_animdata_curves(id);

	if (check_id_has_anim_component(id)) {
		ComponentKey animation_key(id, NodeType::ANIMATION);
		ComponentKey parameters_key(id, NodeType::PARAMETERS);
		add_relation(animation_key, parameters_key, "Animation -> Parameters");
	}
}

void DepsgraphRelationBuilder::build_parameters(ID *id) {
	OperationKey parameters_entry_key(id, NodeType::PARAMETERS, OperationCode::PARAMETERS_ENTRY);
	OperationKey parameters_eval_key(id, NodeType::PARAMETERS, OperationCode::PARAMETERS_EVAL);
	OperationKey parameters_exit_key(id, NodeType::PARAMETERS, OperationCode::PARAMETERS_EXIT);
	add_relation(parameters_entry_key, parameters_eval_key, "Entry -> Eval");
	add_relation(parameters_eval_key, parameters_exit_key, "Entry -> Exit");
}

void DepsgraphRelationBuilder::build_action(Action *action) {
	if (built_map_.checkIsBuiltAndTag(action)) {
		return;
	}
	build_idproperties(action->id.properties);
	if (action->totslot) {
		TimeSourceKey time_src_key;
		ComponentKey animation_key(&action->id, NodeType::ANIMATION);
		add_relation(time_src_key, animation_key, "TimeSrc -> Animation");
	}
}

void DepsgraphRelationBuilder::build_armature(Armature *armature) {
	if (built_map_.checkIsBuiltAndTag(armature)) {
		return;
	}
	build_idproperties(armature->id.properties);
	build_animdata(&armature->id);
	build_parameters(&armature->id);
	build_armature_bones(&armature->bonebase);
}

void DepsgraphRelationBuilder::build_armature_bones(ListBase *bones) {
	LISTBASE_FOREACH(Bone *, bone, bones) {
		build_idproperties(bone->prop);
		build_armature_bones(&bone->childbase);
	}
}

void DepsgraphRelationBuilder::build_collection(LayerCollection *from_layer_collection, Object *object, Collection *collection) {
	if (from_layer_collection != nullptr) {
		/* If we came from layer collection we don't go deeper, view layer
		 * builder takes care of going deeper.
		 *
		 * NOTE: Do early output before tagging build as done, so possible
		 * subsequent builds from outside of the layer collection properly
		 * recurses into all the nested objects and collections. */
		return;
	}
	const bool group_done = built_map_.checkIsBuiltAndTag(collection);
	OperationKey object_transform_final_key(object != nullptr ? &object->id : nullptr, NodeType::TRANSFORM, OperationCode::TRANSFORM_FINAL);
	ComponentKey duplicator_key(object != nullptr ? &object->id : nullptr, NodeType::DUPLI);
	if (!group_done) {
		build_idproperties(collection->id.properties);
		OperationKey collection_geometry_key{&collection->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL_DONE};
		LISTBASE_FOREACH(CollectionObject *, cob, &collection->objects) {
			build_object(cob->object);

			/* The geometry of a collection depends on the positions of the elements in it. */
			OperationKey object_transform_key{&cob->object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_FINAL};
			add_relation(object_transform_key, collection_geometry_key, "Collection Geometry");

			/* Only create geometry relations to child objects, if they have a geometry component. */
			OperationKey object_geometry_key{&cob->object->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL};
			if (find_node(object_geometry_key) != nullptr) {
				add_relation(object_geometry_key, collection_geometry_key, "Collection Geometry");
			}

			/* An instance is part of the geometry of the collection. */
			if (cob->object->type == OB_EMPTY) {
				Collection *collection_instance = cob->object->instance_collection;
				if (collection_instance != nullptr) {
					OperationKey collection_instance_key{&collection_instance->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL_DONE};
					add_relation(collection_instance_key, collection_geometry_key, "Collection Geometry");
				}
			}
		}
		LISTBASE_FOREACH(CollectionChild *, child, &collection->children) {
			build_collection(nullptr, nullptr, child->collection);
			OperationKey child_collection_geometry_key{&child->collection->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL_DONE};
			add_relation(child_collection_geometry_key, collection_geometry_key, "Collection Geometry");
		}
	}
	if (object != nullptr) {
		ROSE_assert_unreachable();
	}
}

void DepsgraphRelationBuilder::build_object(Object *object) {
	if (built_map_.checkIsBuiltAndTag(object)) {
		return;
	}
	/* Object Transforms */
	OperationCode base_op = (object->parent) ? OperationCode::TRANSFORM_PARENT : OperationCode::TRANSFORM_LOCAL;
	OperationKey base_op_key(&object->id, NodeType::TRANSFORM, base_op);
	OperationKey init_transform_key(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_INIT);
	OperationKey local_transform_key(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_LOCAL);
	OperationKey parent_transform_key(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_PARENT);
	OperationKey transform_eval_key(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_EVAL);
	OperationKey final_transform_key(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_FINAL);
	OperationKey ob_eval_key(&object->id, NodeType::TRANSFORM, OperationCode::TRANSFORM_EVAL);
	add_relation(init_transform_key, local_transform_key, "Transform Init");
	/* Various flags, flushing from bases/collections. */
	build_object_layer_component_relations(object);
	/* Parenting. */
	if (object->parent != nullptr) {
		/* Make sure parent object's relations are built. */
		build_object(object->parent);
		/* Parent relationship. */
		build_object_parent(object);
		/* Local -> parent. */
		add_relation(local_transform_key, parent_transform_key, "ObLocal -> ObParent");
	}
	/* Modifiers. */
	if (object->modifiers.first != nullptr) {
		BuilderWalkUserData data;
		data.builder = this;
		KER_modifiers_foreach_ID_link(object, modifier_walk, &data);
	}

	add_relation(base_op_key, ob_eval_key, "Eval");
	add_relation(ob_eval_key, final_transform_key, "Eval -> Final Transform");

	build_idproperties(object->id.properties);
	/* Animation data */
	build_animdata(&object->id);
	/* Object data. */
	build_object_data(object);
	/* Synchronization back to original object. */
	OperationKey synchronize_key(&object->id, NodeType::SYNCHRONIZATION, OperationCode::SYNCHRONIZE_TO_ORIGINAL);
	add_relation(final_transform_key, synchronize_key, "Synchronize to Original");
	/* Parameters. */
	build_parameters(&object->id);
}

void DepsgraphRelationBuilder::build_object_dimensions(Object *object) {
	OperationKey dimensions_key(&object->id, NodeType::PARAMETERS, OperationCode::DIMENSIONS);
	ComponentKey geometry_key(&object->id, NodeType::GEOMETRY);
	ComponentKey transform_key(&object->id, NodeType::TRANSFORM);
	add_relation(geometry_key, dimensions_key, "Geometry -> Dimensions");
	add_relation(transform_key, dimensions_key, "Transform -> Dimensions");
}

void DepsgraphRelationBuilder::build_object_from_view_layer_base(Object *object) {
	/* It is possible to have situation when an object is pulled into the dependency graph in a
	 * few different ways:
	 *
	 *  - Indirect driver dependency, which doesn't have a Base (or, Base is unknown).
	 *  - Via a base from a view layer (view layer of the graph, or view layer of a set scene).
	 *  - Possibly other ways, which are not important for decision making here.
	 *
	 * There needs to be a relation from view layer which has a base with the object so that the
	 * order of flags evaluation is correct (object-level base flags evaluation requires view layer
	 * to be evaluated first).
	 *
	 * This build call handles situation when object comes from a view layer, hence has a base, and
	 * needs a relation from the view layer. Do the relation prior to check of whether the object
	 * relations are built so that the relation is created from every view layer which has a base
	 * with this object. */

	OperationKey view_layer_done_key(&scene_->id, NodeType::LAYER_COLLECTIONS, OperationCode::VIEW_LAYER_EVAL);
	OperationKey object_from_layer_entry_key(&object->id, NodeType::OBJECT_FROM_LAYER, OperationCode::OBJECT_FROM_LAYER_ENTRY);

	add_relation(view_layer_done_key, object_from_layer_entry_key, "View Layer flags to Object");

	/* Regular object building. */
	build_object(object);
}

void DepsgraphRelationBuilder::build_object_layer_component_relations(Object *object) {
	OperationKey object_from_layer_entry_key(&object->id, NodeType::OBJECT_FROM_LAYER, OperationCode::OBJECT_FROM_LAYER_ENTRY);
	OperationKey object_from_layer_exit_key(&object->id, NodeType::OBJECT_FROM_LAYER, OperationCode::OBJECT_FROM_LAYER_EXIT);
	OperationKey object_flags_key(&object->id, NodeType::OBJECT_FROM_LAYER, OperationCode::OBJECT_BASE_FLAGS);

	if (!has_node(object_flags_key)) {
		/* Just connect Entry -> Exit if there is no OBJECT_BASE_FLAGS node. */
		add_relation(object_from_layer_entry_key, object_from_layer_exit_key, "Object from Layer");
		return;
	}

	/* Entry -> OBJECT_BASE_FLAGS -> Exit */
	add_relation(object_from_layer_entry_key, object_flags_key, "Base flags flush Entry");
	add_relation(object_flags_key, object_from_layer_exit_key, "Base flags flush Exit");

	/* Synchronization back to original object. */
	OperationKey synchronize_key(&object->id, NodeType::SYNCHRONIZATION, OperationCode::SYNCHRONIZE_TO_ORIGINAL);
	add_relation(object_from_layer_exit_key, synchronize_key, "Synchronize to Original");
}

void DepsgraphRelationBuilder::build_object_data(Object *object) {
	if (object->data == nullptr) {
		return;
	}
	ID *obdata_id = (ID *)object->data;
	/* Object data animation. */
	if (!built_map_.checkIsBuilt(obdata_id)) {
		build_animdata(obdata_id);
	}
	/* type-specific data. */
	switch (object->type) {
		case OB_MESH: {
			build_object_data_geometry(object);
		} break;
		case OB_ARMATURE: {
			build_object_data_armature(object);
		} break;
		case OB_CAMERA: {
			// TODO;
		} break;
	}
}

void DepsgraphRelationBuilder::build_object_parent(Object *object) {
	Object *parent = object->parent;
	ID *parent_id = &object->parent->id;
	ComponentKey object_transform_key(&object->id, NodeType::TRANSFORM);
	/* Type-specific links. */
	switch (object->partype) {
		/* Armature Deform (Virtual Modifier) */
		case PARSKEL: {
			ComponentKey parent_transform_key(parent_id, NodeType::TRANSFORM);
			add_relation(parent_transform_key, object_transform_key, "Parent Armature Transform");

			if (parent->type == OB_ARMATURE) {
				ComponentKey object_geometry_key(&object->id, NodeType::GEOMETRY);
				ComponentKey parent_pose_key(parent_id, NodeType::EVAL_POSE);
				add_relation(parent_transform_key, object_geometry_key, "Parent Armature Transform -> Geometry");
				add_relation(parent_pose_key, object_geometry_key, "Parent Armature Pose -> Geometry");

				add_depends_on_transform_relation(&object->id, object_geometry_key, "Virtual Armature Modifier");
			}

			break;
		}

		/* Bone Parent */
		case PARBONE: {
			if (object->parsubstr[0] != '\0') {
				ComponentKey parent_bone_key(parent_id, NodeType::BONE, object->parsubstr);
				OperationKey parent_transform_key(parent_id, NodeType::TRANSFORM, OperationCode::TRANSFORM_FINAL);
				add_relation(parent_bone_key, object_transform_key, "Bone Parent");
				add_relation(parent_transform_key, object_transform_key, "Armature Parent");
			}
			break;
		}

		default: {
			/* Standard Parent. */
			ComponentKey parent_key(parent_id, NodeType::TRANSFORM);
			add_relation(parent_key, object_transform_key, "Parent");
			break;
		}
	}
}

void DepsgraphRelationBuilder::build_object_data_armature(Object *object) {
	/* Armature-Data */
	Armature *armature = (Armature *)object->data;
	// TODO: selection status?
	/* Attach links between pose operations. */
	ComponentKey local_transform(&object->id, NodeType::TRANSFORM);
	OperationKey pose_init_key(&object->id, NodeType::EVAL_POSE, OperationCode::POSE_INIT);
	OperationKey pose_init_ik_key(&object->id, NodeType::EVAL_POSE, OperationCode::POSE_INIT_IK);
	OperationKey pose_cleanup_key(&object->id, NodeType::EVAL_POSE, OperationCode::POSE_CLEANUP);
	OperationKey pose_done_key(&object->id, NodeType::EVAL_POSE, OperationCode::POSE_DONE);
	add_relation(local_transform, pose_init_key, "Local Transform -> Pose Init");
	add_relation(pose_init_key, pose_init_ik_key, "Pose Init -> Pose Init IK");
	add_relation(pose_init_ik_key, pose_done_key, "Pose Init IK -> Pose Cleanup");
	/* Make sure pose is up-to-date with armature updates. */
	build_armature(armature);
	OperationKey armature_key(&armature->id, NodeType::ARMATURE, OperationCode::ARMATURE_EVAL);
	add_relation(armature_key, pose_init_key, "Data dependency");
	/* Run cleanup even when there are no bones. */
	add_relation(pose_init_key, pose_cleanup_key, "Init -> Cleanup");

	LISTBASE_FOREACH(PoseChannel *, pchannel, &object->pose->channelbase) {
		OperationKey bone_local_key(&object->id, NodeType::BONE, pchannel->name, OperationCode::BONE_LOCAL);
		OperationKey bone_pose_key(&object->id, NodeType::BONE, pchannel->name, OperationCode::BONE_POSE_PARENT);
		OperationKey bone_ready_key(&object->id, NodeType::BONE, pchannel->name, OperationCode::BONE_READY);
		OperationKey bone_done_key(&object->id, NodeType::BONE, pchannel->name, OperationCode::BONE_DONE);
		pchannel->flag &= ~POSE_DONE;
		/* Pose init to bone local. */
		add_relation(pose_init_key, bone_local_key, "Pose Init - Bone Local", RELATION_FLAG_GODMODE);
		/* Local to pose parenting operation. */
		add_relation(bone_local_key, bone_pose_key, "Bone Local - Bone Pose");
		/* Parent relation. */
		if (pchannel->parent != nullptr) {
			OperationKey parent_key(&object->id, NodeType::BONE, pchannel->parent->name, OperationCode::BONE_DONE);
			add_relation(parent_key, bone_pose_key, "Parent Bone -> Child Bone");
			/* Pose -> Ready */
			add_relation(bone_pose_key, bone_ready_key, "Pose -> Ready");
			/* Bone ready -> Bone done.
			 * NOTE: For bones without IK, this is all that's needed.
			 *       For IK chains however, an additional rel is created from IK
			 *       to done, with transitive reduction removing this one. */
			add_relation(bone_ready_key, bone_done_key, "Ready -> Done");
			/* Assume that all bones must be done for the pose to be ready
			 * (for deformers). */
			add_relation(bone_done_key, pose_done_key, "PoseEval Result-Bone Link");
			/* Bones must be traversed before cleanup. */
			add_relation(bone_done_key, pose_cleanup_key, "Done -> Cleanup");
			add_relation(bone_ready_key, pose_cleanup_key, "Ready -> Cleanup");
		}
	}
}

void DepsgraphRelationBuilder::build_object_data_geometry(Object *object) {
	ID *obdata = (ID *)object->data;
	/* Init operation of object-level geometry evaluation. */
	OperationKey geom_init_key(&object->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL_INIT);
	/* Get nodes for result of obdata's evaluation, and geometry evaluation
	 * on object. */
	ComponentKey obdata_geom_key(obdata, NodeType::GEOMETRY);
	ComponentKey geom_key(&object->id, NodeType::GEOMETRY);
	/* Link components to each other. */
	add_relation(obdata_geom_key, geom_key, "Object Geometry Base Data");
	OperationKey obdata_ubereval_key(&object->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL);
	/* Modifiers */
	if (object->modifiers.first != nullptr) {
		LISTBASE_FOREACH(ModifierData *, md, &object->modifiers) {
			const ModifierTypeInfo *mti = KER_modifier_get_info((eModifierType)md->type);

			// TODO;
		}
	}
	/* Make sure uber update is the last in the dependencies. */
	if (object->type != OB_ARMATURE) {
		/* Armatures does no longer require uber node. */
		OperationKey obdata_ubereval_key(&object->id, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL);
		add_relation(geom_init_key, obdata_ubereval_key, "Object Geometry UberEval");
	}
	/* Object data data-block. */
	build_object_data_geometry_datablock((ID *)object->data);
	build_object_dimensions(object);
	/* Synchronization back to original object. */
	ComponentKey final_geometry_key(&object->id, NodeType::GEOMETRY);
	OperationKey synchronize_key(&object->id, NodeType::SYNCHRONIZATION, OperationCode::SYNCHRONIZE_TO_ORIGINAL);
	add_relation(final_geometry_key, synchronize_key, "Synchronize to Original");
}

void DepsgraphRelationBuilder::build_object_data_geometry_datablock(ID *obdata) {
	if (built_map_.checkIsBuiltAndTag(obdata)) {
		return;
	}
	build_idproperties(obdata->properties);
	/* Animation. */
	build_animdata(obdata);
	build_parameters(obdata);
	/* Link object data evaluation node to exit operation. */
	OperationKey obdata_geom_eval_key(obdata, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL);
	OperationKey obdata_geom_done_key(obdata, NodeType::GEOMETRY, OperationCode::GEOMETRY_EVAL_DONE);
	add_relation(obdata_geom_eval_key, obdata_geom_done_key, "ObData Geom Eval Done");

	/* Link object data evaluation to parameter evaluation. */
	ComponentKey parameters_key(obdata, NodeType::PARAMETERS);
	add_relation(parameters_key, obdata_geom_eval_key, "ObData Geom Params");

	/* Type-specific links. */
	const ID_Type id_type = GS(obdata->name);
	switch (id_type) {
		case ID_ME:
			break;
		default:
			ROSE_assert_msg(0, "Should not happen");
			break;
	}
}

void DepsgraphRelationBuilder::build_scene_parameters(Scene *scene) {
	if (built_map_.checkIsBuiltAndTag(scene, BuilderMap::TAG_PARAMETERS)) {
		return;
	}
	build_idproperties(scene->id.properties);
	build_parameters(&scene->id);
	OperationKey parameters_eval_key(&scene->id, NodeType::PARAMETERS, OperationCode::PARAMETERS_EXIT);
	OperationKey scene_eval_key(&scene->id, NodeType::PARAMETERS, OperationCode::SCENE_EVAL);
	add_relation(parameters_eval_key, scene_eval_key, "Parameters -> Scene Eval");
}

void DepsgraphRelationBuilder::build_view_layer(Scene *scene, ViewLayer *view_layer, eDepsNode_LinkedState_Type linked_state) {
	/* Setup currently building context. */
	scene_ = scene;
	/* Scene objects. */
	/* NOTE: Nodes builder requires us to pass CoW base because it's being
	 * passed to the evaluation functions. During relations builder we only
	 * do nullptr-pointer check of the base, so it's fine to pass original one. */
	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		if (need_pull_base_into_graph(base)) {
			build_object_from_view_layer_base(base->object);
		}
	}

	build_layer_collections(&view_layer->layer_collections);

	if (scene->camera != nullptr) {
		build_object(scene->camera);
	}
	/* Scene's animation and drivers. */
	if (scene->adt != nullptr) {
		build_animdata(&scene->id);
	}
	/* Scene parameters, compositor and such. */
	build_scene_parameters(scene);
	/* Make final scene evaluation dependent on view layer evaluation. */
	OperationKey scene_view_layer_key(&scene->id, NodeType::LAYER_COLLECTIONS, OperationCode::VIEW_LAYER_EVAL);
	OperationKey scene_eval_key(&scene->id, NodeType::PARAMETERS, OperationCode::SCENE_EVAL);
	add_relation(scene_view_layer_key, scene_eval_key, "View Layer -> Scene Eval");
}

void DepsgraphRelationBuilder::build_layer_collections(ListBase *lb) {
	for (LayerCollection *lc = (LayerCollection *)lb->first; lc; lc = lc->next) {
		if (lc->collection->flag & COLLECTION_HIDE_VIEWPORT) {
			continue;
		}
		if ((lc->flag & LAYER_COLLECTION_EXCLUDE) == 0) {
			build_collection(lc, nullptr, lc->collection);
		}
		build_layer_collections(&lc->layer_collections);
	}
}

TimeSourceNode *DepsgraphRelationBuilder::get_node(const TimeSourceKey &key) const {
	if (key.id) {
		/* XXX TODO */
		return nullptr;
	}

	return graph_->time_source;
}

ComponentNode *DepsgraphRelationBuilder::get_node(const ComponentKey &key) const {
	IDNode *id_node = graph_->find_id_node(key.id);
	if (!id_node) {
		fprintf(stderr, "find_node component: Could not find ID %s\n", (key.id != nullptr) ? key.id->name : "<null>");
		return nullptr;
	}

	ComponentNode *node = id_node->find_component(key.type, key.name);
	return node;
}

OperationNode *DepsgraphRelationBuilder::get_node(const OperationKey &key) const {
	OperationNode *op_node = find_node(key);
	if (op_node == nullptr) {
		fprintf(stderr, "find_node_operation: Failed for (%s, '%s')\n", DEG_operation_code_as_string(key.opcode), key.name);
	}
	return op_node;
}

Node *DepsgraphRelationBuilder::get_node(const RNAPathKey &key) {
	return rna_node_query_.find_node(&key.ptr, key.prop, key.source);
}

OperationNode *DepsgraphRelationBuilder::find_node(const OperationKey &key) const {
	IDNode *id_node = graph_->find_id_node(key.id);
	if (!id_node) {
		return nullptr;
	}
	ComponentNode *comp_node = id_node->find_component(key.component_type, key.component_name);
	if (!comp_node) {
		return nullptr;
	}
	return comp_node->find_operation(key.opcode, key.name, key.name_tag);
}

bool DepsgraphRelationBuilder::has_node(const OperationKey &key) const {
	return find_node(key) != nullptr;
}

Relation *DepsgraphRelationBuilder::add_time_relation(TimeSourceNode *timesrc, Node *node_to, const char *description, int flags) {
	if (timesrc && node_to) {
		return graph_->add_new_relation(timesrc, node_to, description, flags);
	}

	return nullptr;
}

Relation *DepsgraphRelationBuilder::add_operation_relation(OperationNode *node_from, OperationNode *node_to, const char *description, int flags) {
	if (node_from && node_to) {
		return graph_->add_new_relation(node_from, node_to, description, flags);
	}

	return nullptr;
}

void DepsgraphRelationBuilder::add_visibility_relation(ID *id_from, ID *id_to) {
	ComponentKey from_key(id_from, NodeType::VISIBILITY);
	ComponentKey to_key(id_to, NodeType::VISIBILITY);
	add_relation(from_key, to_key, "visibility");
}

void DepsgraphRelationBuilder::modifier_walk(void *user_data, struct Object *object, struct ID **idpoin, int flag) {
	BuilderWalkUserData *data = (BuilderWalkUserData *)user_data;
	ID *id = *idpoin;
	if (id == nullptr) {
		return;
	}
	data->builder->build_id(id);
}

}  // namespace rose::depsgraph
