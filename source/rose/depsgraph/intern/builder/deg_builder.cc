#include "DNA_ID.h"
#include "DNA_anim_types.h"
#include "DNA_armature_types.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"

#include "deg_builder.hh"

#include "LIB_stack.h"
#include "LIB_utildefines.h"

#include "KER_action.h"

#include "RNA_prototypes.h"

#include "intern/builder/deg_builder_cache.hh"
#include "intern/builder/deg_builder_remove_noop.hh"
#include "intern/depsgraph.hh"
#include "intern/depsgraph_relation.hh"
#include "intern/depsgraph_tag.hh"
#include "intern/depsgraph_types.hh"
#include "intern/eval/deg_eval_copy_on_write.hh"
#include "intern/node/deg_node.hh"
#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"

namespace rose::depsgraph {

bool deg_check_id_in_depsgraph(const Depsgraph *graph, ID *id_orig) {
	IDNode *id_node = graph->find_id_node(id_orig);
	return id_node != nullptr;
}

bool deg_check_base_in_depsgraph(const Depsgraph *graph, Base *base) {
	Object *object_orig = base->runtime.base_orig->object;
	IDNode *id_node = graph->find_id_node(&object_orig->id);
	if (id_node == nullptr) {
		return false;
	}
	return id_node->has_base;
}

DepsgraphBuilder::DepsgraphBuilder(Main *main, Depsgraph *graph, DepsgraphBuilderCache *cache) : main_(main), graph_(graph), cache_(cache) {
}

bool DepsgraphBuilder::need_pull_base_into_graph(Base *base) {
	/* Simple check: enabled bases are always part of dependency graph. */
	const int base_flag = BASE_ENABLED_VIEWPORT;
	if (base->flag & base_flag) {
		return true;
	}
	/**
	 * More involved check: since we don't support dynamic changes in dependency graph topology and
	 * all visible objects are to be part of dependency graph, we pull all objects which has animated
	 * visibility.
	 */
	Object *object = base->object;
	AnimatedPropertyID property_id = AnimatedPropertyID(&object->id, &RNA_Object, "hide_viewport");
	return cache_->isPropertyAnimated(&object->id, property_id);
}

namespace {

void deg_graph_build_flush_visibility(Depsgraph *graph) {
	enum {
		DEG_NODE_VISITED = (1 << 0),
	};

	RoseStack *stack = LIB_stack_new(sizeof(OperationNode *), "DEG flush layers stack");
	for (IDNode *id_node : graph->id_nodes) {
		for (ComponentNode *comp_node : id_node->components.values()) {
			comp_node->affects_directly_visible |= id_node->is_directly_visible;

			/**
			 * Enforce "visibility" of the syncronization component.
			 *
			 * This component is never connected to other ID nodes, and hence can not be handled in the
			 * same way as other components needed for evaluation. It is only needed for proper
			 * evaluation of the ID node it belongs to.
			 *
			 * The design is such that the synchronization is supposed to happen whenever any part of the
			 * ID changed/evaluated. Here we mark the component as "visible" so that genetic recalc flag
			 * flushing and scheduling will handle the component in a generic manner.
			 */
			if (comp_node->type == NodeType::SYNCHRONIZATION) {
				comp_node->affects_directly_visible = true;
			}
		}
	}

	for (OperationNode *op_node : graph->operations) {
		op_node->custom_flags = 0;
		op_node->num_links_pending = 0;
		for (Relation *rel : op_node->outlinks) {
			if ((rel->from->type == NodeType::OPERATION) && (rel->flag & RELATION_FLAG_CYCLIC) == 0) {
				++op_node->num_links_pending;
			}
		}
		if (op_node->num_links_pending == 0) {
			LIB_stack_push(stack, &op_node);
			op_node->custom_flags |= DEG_NODE_VISITED;
		}
	}

	while (!LIB_stack_is_empty(stack)) {
		OperationNode *op_node;
		LIB_stack_pop(stack, &op_node);
		/* Flush layers to parents. */
		for (Relation *rel : op_node->inlinks) {
			if (rel->from->type == NodeType::OPERATION) {
				OperationNode *op_from = (OperationNode *)rel->from;
				ComponentNode *comp_from = op_from->owner;
				const bool target_directly_visible = op_node->owner->affects_directly_visible;

				/**
				 * Visibility component forces all components of the current ID to be considered as
				 * affecting directly visible.
				 */
				if (comp_from->type == NodeType::VISIBILITY) {
					if (target_directly_visible) {
						IDNode *id_node_from = comp_from->owner;
						for (ComponentNode *comp_node : id_node_from->components.values()) {
							comp_node->affects_directly_visible |= target_directly_visible;
						}
					}
				}
				else {
					comp_from->affects_directly_visible |= target_directly_visible;
				}
			}
		}
		/* Schedule parent nodes. */
		for (Relation *rel : op_node->inlinks) {
			if (rel->from->type == NodeType::OPERATION) {
				OperationNode *op_from = (OperationNode *)rel->from;
				if ((rel->flag & RELATION_FLAG_CYCLIC) == 0) {
					ROSE_assert(op_from->num_links_pending > 0);
					--op_from->num_links_pending;
				}
				if ((op_from->num_links_pending == 0) && (op_from->custom_flags & DEG_NODE_VISITED) == 0) {
					LIB_stack_push(stack, &op_from);
					op_from->custom_flags |= DEG_NODE_VISITED;
				}
			}
		}
	}
	LIB_stack_free(stack);
}

}  // namespace

void deg_graph_build_finalize(Main *main, Depsgraph *graph) {
	/* Make sure dependencies of visible ID datablocks are visible. */
	deg_graph_build_flush_visibility(graph);
	deg_graph_remove_unused_noops(graph);

	/**
	 * Re-tag IDs for update if it was tagged before the relations
	 * update tag.
	 */
	for (IDNode *id_node : graph->id_nodes) {
		ID *id_orig = id_node->id_orig;
		id_node->finalize_build(graph);
		int flag = 0;
		/* Tag rebuild if special evaluation flags changed. */
		if (id_node->eval_flags != id_node->previous_eval_flags) {
			flag |= ID_RECALC_TRANSFORM | ID_RECALC_GEOMETRY;
		}
		/* Tag rebuild if the custom data mask changed. */
		if (id_node->customdata_masks != id_node->previous_customdata_masks) {
			flag |= ID_RECALC_GEOMETRY;
		}
		if (!deg_copy_on_write_is_expanded(id_node->id_cow)) {
			flag |= ID_RECALC_COPY_ON_WRITE;
			/**
			 * This means ID is being added to the dependency graph first
			 * time, which is similar to "ob-visible-change"
			 */
			if (GS(id_orig->name) == ID_OB) {
				flag |= ID_RECALC_TRANSFORM | ID_RECALC_GEOMETRY;
			}
		}
		/**
		 * Restore recalc flags from original ID, which could possibly contain recalc flags set by
		 * an operator and then were carried on by the undo system.
		 */
		flag |= id_orig->recalc;
		if (flag != 0) {
			graph_id_tag_update(main, graph, id_node->id_orig, flag, DEG_UPDATE_SOURCE_RELATIONS);
		}
	}
}

}  // namespace rose::depsgraph
