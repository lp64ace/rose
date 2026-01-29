#include "DNA_userdef_types.h"

#include "LIB_math_bit.h"

#include "KER_anim_data.h"
#include "KER_global.h"
#include "KER_idtype.h"
#include "KER_main.h"
#include "KER_userdef.h"

#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_factory.hh"
#include "intern/node/deg_node_id.hh"

#include "intern/eval/deg_eval_copy_on_write.hh"

#include "depsgraph.hh"
#include "depsgraph_registry.hh"
#include "depsgraph_types.hh"

namespace rose::depsgraph {

NodeType geometry_tag_to_component(const ID *id) {
	const ID_Type id_type = GS(id->name);
	switch (id_type) {
		case ID_OB: {
			const Object *object = (Object *)id;
			switch (object->type) {
				case OB_MESH:
					return NodeType::GEOMETRY;
				case OB_ARMATURE:
					return NodeType::EVAL_POSE;
			}
			break;
		}
		case ID_ME:
		case ID_GR:
			return NodeType::GEOMETRY;
		default:
			break;
	}
	return NodeType::UNDEFINED;
}

void depsgraph_geometry_tag_to_component(const ID *id, NodeType *component_type) {
	const NodeType result = geometry_tag_to_component(id);
	if (result != NodeType::UNDEFINED) {
		*component_type = result;
	}
}

int deg_recalc_flags_for_legacy_zero() {
	return ID_RECALC_ALL & ~(ID_RECALC_ANIMATION | ID_RECALC_FRAME_CHANGE);
}

int deg_recalc_flags_effective(Depsgraph *graph, int flags) {
	if (graph != nullptr) {
		if (!graph->is_active) {
			return 0;
		}
	}
	if (flags == 0) {
		return deg_recalc_flags_for_legacy_zero();
	}
	return flags;
}

/**
 * Special tag function which tags all components which needs to be tagged
 * for update flag=0.
 */
void deg_graph_node_tag_zero(Main *bmain, Depsgraph *graph, IDNode *id_node, eUpdateSource update_source) {
	if (id_node == nullptr) {
		return;
	}
	ID *id = id_node->id_orig;
	/* TODO; Which recalc flags to set here? */
	id_node->id_cow->recalc |= deg_recalc_flags_for_legacy_zero();

	for (ComponentNode *comp_node : id_node->components.values()) {
		if (comp_node->type == NodeType::ANIMATION) {
			continue;
		}
		if (comp_node->type == NodeType::COPY_ON_WRITE) {
			id_node->is_cow_explicitly_tagged = true;
		}

		comp_node->tag_update(graph, update_source);
	}
}

void depsgraph_tag_to_component_opcode(const ID *id, int recalc, NodeType *component_type, OperationCode *operation_code) {
	const ID_Type id_type = GS(id->name);
	*component_type = NodeType::UNDEFINED;
	*operation_code = OperationCode::OPERATION;
	/* Special case for now, in the future we should get rid of this. */
	if (recalc == 0) {
		*component_type = NodeType::ID_REF;
		*operation_code = OperationCode::OPERATION;
		return;
	}
	switch (recalc) {
		case ID_RECALC_TRANSFORM:
			*component_type = NodeType::TRANSFORM;
			break;
		case ID_RECALC_GEOMETRY:
			depsgraph_geometry_tag_to_component(id, component_type);
			break;
		case ID_RECALC_ANIMATION:
			*component_type = NodeType::ANIMATION;
			break;
		case ID_RECALC_COPY_ON_WRITE:
			*component_type = NodeType::COPY_ON_WRITE;
			break;
		case ID_RECALC_ALL:
			ROSE_assert_msg(0, "Should not happen");
			break;
	}
}

void depsgraph_id_tag_copy_on_write(Depsgraph *graph, IDNode *id_node, eUpdateSource update_source) {
	ComponentNode *cow_comp = id_node->find_component(NodeType::COPY_ON_WRITE);
	if (cow_comp == nullptr) {
		ROSE_assert(!deg_copy_on_write_is_needed(GS(id_node->id_orig->name)));
		return;
	}
	cow_comp->tag_update(graph, update_source);
}

void depsgraph_tag_component(Depsgraph *graph, IDNode *id_node, NodeType component_type, OperationCode operation_code, eUpdateSource update_source) {
	ComponentNode *component_node = id_node->find_component(component_type);
	/* NOTE: Animation component might not be existing yet (which happens when adding new driver or
	 * adding a new keyframe), so the required copy-on-write tag needs to be taken care explicitly
	 * here. */
	if (component_node == nullptr) {
		if (component_type == NodeType::ANIMATION) {
			id_node->is_cow_explicitly_tagged = true;
			depsgraph_id_tag_copy_on_write(graph, id_node, update_source);
		}
		return;
	}
	if (operation_code == OperationCode::OPERATION) {
		component_node->tag_update(graph, update_source);
	}
	else {
		OperationNode *operation_node = component_node->find_operation(operation_code);
		if (operation_node != nullptr) {
			operation_node->tag_update(graph, update_source);
		}
	}
	/* If component depends on copy-on-write, tag it as well. */
	if (component_node->need_tag_cow_before_update()) {
		depsgraph_id_tag_copy_on_write(graph, id_node, update_source);
	}
	if (component_type == NodeType::COPY_ON_WRITE) {
		id_node->is_cow_explicitly_tagged = true;
	}
}

void graph_id_tag_update_single_flag(Main *bmain, Depsgraph *graph, ID *id, IDNode *id_node, int recalc, eUpdateSource update_source) {
	/* Get description of what is to be tagged. */
	NodeType component_type;
	OperationCode operation_code;
	depsgraph_tag_to_component_opcode(id, recalc, &component_type, &operation_code);
	/* Check whether we've got something to tag. */
	if (component_type == NodeType::UNDEFINED) {
		/* Given ID does not support tag. */
		/* TODO(sergey): Shall we raise some panic here? */
		return;
	}
	/* Some sanity checks before moving forward. */
	if (id_node == nullptr) {
		/* Happens when object is tagged for update and not yet in the
		 * dependency graph (but will be after relations update). */
		return;
	}
	/* Tag ID recalc flag. */
	DepsNodeFactory *factory = type_get_factory(component_type);
	ROSE_assert(factory != nullptr);
	id_node->id_cow->recalc |= factory->id_recalc_tag();
	/* Tag corresponding dependency graph operation for update. */
	if (component_type == NodeType::ID_REF) {
		id_node->tag_update(graph, update_source);
	}
	else {
		depsgraph_tag_component(graph, id_node, component_type, operation_code, update_source);
	}
}

void graph_id_tag_update(Main *main, Depsgraph *graph, ID *id, int flag, eUpdateSource update_source) {
	if (graph != nullptr && graph->is_evaluating) {
		return;
	}
	IDNode *id_node = (graph != nullptr) ? graph->find_id_node(id) : nullptr;
	if (graph != nullptr) {
		DEG_graph_id_type_tag(reinterpret_cast<::Depsgraph *>(graph), GS(id->name));
	}
	if (flag == 0) {
		deg_graph_node_tag_zero(main, graph, id_node, update_source);
	}
	/* Store original flag in the ID.
	 * Allows to have more granularity than a node-factory based flags. */
	if (id_node != nullptr) {
		id_node->id_cow->recalc |= flag;
	}
	/* When ID is tagged for update based on an user edits store the recalc flags in the original ID.
	 * This way IDs in the undo steps will have this flag preserved, making it possible to restore
	 * all needed tags when new dependency graph is created on redo.
	 * This is the only way to ensure modifications to animation data (such as keyframes i.e.)
	 * properly triggers animation update for the newly constructed dependency graph on redo (while
	 * usually newly created dependency graph skips animation update to avoid loss of unkeyed
	 * changes). */
	if (update_source & DEG_UPDATE_SOURCE_USER_EDIT) {
		id->recalc |= deg_recalc_flags_effective(graph, flag);
	}
	unsigned int current_flag = flag;
	while (current_flag != 0) {
		int recalc = (1 << _lib_forward_scan_clear_u32(&current_flag));
		graph_id_tag_update_single_flag(main, graph, id, id_node, recalc, update_source);
	}
}

void id_tag_update(Main *main, ID *id, int flag, eUpdateSource update_source) {
	graph_id_tag_update(main, nullptr, id, flag, update_source);
	for (rose::depsgraph::Depsgraph *depsgraph : rose::depsgraph::get_all_registered_graphs(main)) {
		graph_id_tag_update(main, depsgraph, id, flag, update_source);
	}
}

void graph_tag_ids_for_visible_update(Depsgraph *graph) {
	if (!graph->need_visibility_update) {
		return;
	}

	const bool do_time = graph->need_visibility_time_update;
	Main *main = graph->main;

	/* NOTE: It is possible to have this function called with `do_time=false` first and later (prior
	 * to evaluation though) with `do_time=true`. This means early output checks should be aware of
	 * this. */
	for (IDNode *id_node : graph->id_nodes) {
		const ID_Type id_type = GS(id_node->id_orig->name);

		if (!id_node->visible_components_mask) {
			/* ID has no components which affects anything visible.
			 * No need bother with it to tag or anything. */
			continue;
		}
		int flag = 0;
		if (!deg_copy_on_write_is_expanded(id_node->id_cow)) {
			flag |= ID_RECALC_COPY_ON_WRITE;
			if (do_time) {
				if (KER_animdata_from_id(id_node->id_orig) != nullptr) {
					flag |= ID_RECALC_ANIMATION;
				}
			}
		}
		else {
			if (id_node->visible_components_mask == id_node->previously_visible_components_mask) {
				/* The ID was already visible and evaluated, all the subsequent
				 * updates and tags are to be done explicitly. */
				continue;
			}
		}
		/* We only tag components which needs an update. Tagging everything is
		 * not a good idea because that might reset particles cache (or any
		 * other type of cache).
		 *
		 * TODO(sergey): Need to generalize this somehow. */
		if (id_type == ID_OB) {
			flag |= ID_RECALC_TRANSFORM | ID_RECALC_GEOMETRY;
		}
		graph_id_tag_update(main, graph, id_node->id_orig, flag, DEG_UPDATE_SOURCE_VISIBILITY);
		if (id_type == ID_SCE) {
			/* Make sure collection properties are up to date. */
			id_node->tag_update(graph, DEG_UPDATE_SOURCE_VISIBILITY);
		}
		/* Now when ID is updated to the new visibility state, prevent it from
		 * being re-tagged again. Simplest way to do so is to pretend that it
		 * was already updated by the "previous" dependency graph.
		 *
		 * NOTE: Even if the on_visible_update() is called from the state when
		 * dependency graph is tagged for relations update, it will be fine:
		 * since dependency graph builder re-schedules entry tags, all the
		 * tags we request from here will be applied in the updated state of
		 * dependency graph. */
		id_node->previously_visible_components_mask = id_node->visible_components_mask;
	}

	graph->need_visibility_update = false;
	graph->need_visibility_time_update = false;
}

void graph_tag_on_visible_update(Depsgraph *graph, const bool do_time) {
	graph->need_visibility_update = true;
	graph->need_visibility_time_update |= do_time;
}

}  // namespace rose::depsgraph

namespace deg = rose::depsgraph;

void DEG_graph_tag_on_visible_update(Depsgraph *depsgraph, bool do_time) {
	deg::Depsgraph *graph = (deg::Depsgraph *)depsgraph;
	deg::graph_tag_on_visible_update(graph, do_time);
}

void DEG_tag_on_visible_update(Main *main, bool do_time) {
	for (deg::Depsgraph *depsgraph : deg::get_all_registered_graphs(main)) {
		deg::graph_tag_on_visible_update(depsgraph, do_time);
	}
}

void DEG_id_tag_update(ID *id, int flag) {
	DEG_id_tag_update_ex(G_MAIN, id, flag);
}

void DEG_id_tag_update_ex(Main *main, ID *id, int flag) {
	if (id == nullptr) {
		/* Ideally should not happen, but old depsgraph allowed this. */
		return;
	}
	deg::id_tag_update(main, id, flag, deg::DEG_UPDATE_SOURCE_USER_EDIT);
}

void DEG_graph_id_tag_update(Main *main, Depsgraph *depsgraph, struct ID *id, int flag) {
	deg::Depsgraph *graph = (deg::Depsgraph *)depsgraph;
	deg::graph_id_tag_update(main, graph, id, flag, deg::DEG_UPDATE_SOURCE_USER_EDIT);
}

void DEG_graph_id_type_tag(Depsgraph *depsgraph, short id_type) {
	const int id_type_index = KER_idtype_idcode_to_index(id_type);
	deg::Depsgraph *deg_graph = reinterpret_cast<deg::Depsgraph *>(depsgraph);
	deg_graph->id_type_updated[id_type_index] = 1;
}

void DEG_id_type_tag(Main *main, short id_type) {
	for (deg::Depsgraph *depsgraph : deg::get_all_registered_graphs(main)) {
		DEG_graph_id_type_tag(reinterpret_cast<::Depsgraph *>(depsgraph), id_type);
	}
}

static void deg_graph_clear_id_recalc_flags(ID *id) {
	id->recalc &= ~ID_RECALC_ALL;
	/* XXX And what about scene's master collection here? */
}

void DEG_ids_clear_recalc(Depsgraph *depsgraph, bool backup) {
	deg::Depsgraph *deg_graph = reinterpret_cast<deg::Depsgraph *>(depsgraph);
	if (!DEG_id_type_any_updated(depsgraph)) {
		return;
	}

	/* Go over all ID nodes, clearing tags. */
	for (deg::IDNode *id_node : deg_graph->id_nodes) {
		if (backup) {
			id_node->id_cow_recalc_backup |= id_node->id_cow->recalc;
		}
		/* TODO: we clear original ID recalc flags here, but this may not work
		 * correctly when there are multiple depsgraph with others still using
		 * the recalc flag. */
		id_node->is_user_modified = false;
		id_node->is_cow_explicitly_tagged = false;
		deg_graph_clear_id_recalc_flags(id_node->id_cow);
		if (deg_graph->is_active) {
			deg_graph_clear_id_recalc_flags(id_node->id_orig);
		}
	}
	memset(deg_graph->id_type_updated, 0, sizeof(deg_graph->id_type_updated));
}