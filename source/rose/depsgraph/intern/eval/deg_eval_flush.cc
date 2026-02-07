#include "LIB_listbase.h"
#include "LIB_math_vector.h"
#include "LIB_task.h"
#include "LIB_utildefines.h"

#include "KER_lib_id.h"
#include "KER_object.h"
#include "KER_scene.h"

#include "DEG_depsgraph.h"

#include "intern/depsgraph.hh"
#include "intern/depsgraph_relation.hh"
#include "intern/depsgraph_types.hh"
#include "intern/depsgraph_update.hh"
#include "intern/node/deg_node.hh"
#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_factory.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"
#include "intern/node/deg_node_time.hh"

#include <deque>

namespace rose::depsgraph {

enum {
  ID_STATE_NONE = 0,
  ID_STATE_MODIFIED = 1,
};

enum {
  COMPONENT_STATE_NONE = 0,
  COMPONENT_STATE_SCHEDULED = 1,
  COMPONENT_STATE_DONE = 2,
};

using FlushQueue = std::deque<OperationNode *>;

namespace {

void flush_init_id_node_func(void *__restrict data_v, const int i, const TaskParallelTLS *__restrict /*tls*/) {
	Depsgraph *graph = (Depsgraph *)data_v;
	IDNode *id_node = graph->id_nodes[i];
	id_node->custom_flags = ID_STATE_NONE;
	for (ComponentNode *comp_node : id_node->components.values()) {
		comp_node->custom_flags = COMPONENT_STATE_NONE;
	}
}

inline void flush_prepare(Depsgraph *graph) {
	for (OperationNode *node : graph->operations) {
		node->scheduled = false;
	}

	{
		const int num_id_nodes = graph->id_nodes.size();
		TaskParallelSettings settings;
		LIB_parallel_range_settings_defaults(&settings);
		settings.min_iter_per_thread = 1024;
		LIB_task_parallel_range(0, num_id_nodes, graph, flush_init_id_node_func, &settings);
	}
}

inline void flush_schedule_entrypoints(Depsgraph *graph, FlushQueue *queue) {
	for (OperationNode *op_node : graph->entry_tags) {
		queue->push_back(op_node);
		op_node->scheduled = true;
	}
}

inline void flush_handle_id_node(IDNode *id_node) {
	id_node->custom_flags = ID_STATE_MODIFIED;
}

inline void flush_handle_component_node(IDNode *id_node, ComponentNode *comp_node, FlushQueue *queue) {
	/* We only handle component once. */
	if (comp_node->custom_flags == COMPONENT_STATE_DONE) {
		return;
	}
	comp_node->custom_flags = COMPONENT_STATE_DONE;
	for (OperationNode *op : comp_node->operations) {
		op->flag |= DEPSOP_FLAG_NEEDS_UPDATE;
	}
	/* when some target changes bone, we might need to re-run the
	 * whole IK solver, otherwise result might be unpredictable. */
	if (comp_node->type == NodeType::BONE) {
		ComponentNode *pose_comp = id_node->find_component(NodeType::EVAL_POSE);
		ROSE_assert(pose_comp != nullptr);
		if (pose_comp->custom_flags == COMPONENT_STATE_NONE) {
			queue->push_front(pose_comp->get_entry_operation());
			pose_comp->custom_flags = COMPONENT_STATE_SCHEDULED;
		}
	}
}

/**
 * Schedule children of the given operation node for traversal.
 *
 * One of the children will by-pass the queue and will be returned as a function
 * return value, so it can start being handled right away, without building too
 * much of a queue.
 */
inline OperationNode *flush_schedule_children(OperationNode *op_node, FlushQueue *queue) {
	if (op_node->flag & DEPSOP_FLAG_USER_MODIFIED) {
		IDNode *id_node = op_node->owner->owner;
		id_node->is_user_modified = true;
	}

	OperationNode *result = nullptr;
	for (Relation *rel : op_node->outlinks) {
		/* Flush is forbidden, completely. */
		if (rel->flag & RELATION_FLAG_NO_FLUSH) {
			continue;
		}
		/* Relation only allows flushes on user changes, but the node was not
		 * affected by user. */
		if ((rel->flag & RELATION_FLAG_FLUSH_USER_EDIT_ONLY) && (op_node->flag & DEPSOP_FLAG_USER_MODIFIED) == 0) {
			continue;
		}
		OperationNode *to_node = (OperationNode *)rel->to;
		/* Always flush flushable flags, so children always know what happened
		 * to their parents. */
		to_node->flag |= (op_node->flag & DEPSOP_FLAG_FLUSH);
		/* Flush update over the relation, if it was not flushed yet. */
		if (to_node->scheduled) {
			continue;
		}
		if (result != nullptr) {
			queue->push_front(to_node);
		}
		else {
			result = to_node;
		}
		to_node->scheduled = true;
	}
	return result;
}

void flush_engine_data_update(ID *id) {
	DrawDataList *draw_data_list = KER_drawdatalist_get(id);
	if (draw_data_list == nullptr) {
		return;
	}
	LISTBASE_FOREACH(DrawData *, draw_data, draw_data_list) {
		draw_data->recalc |= id->recalc;
	}
}

/* NOTE: It will also accumulate flags from changed components. */
void flush_editors_id_update(Depsgraph *graph, const DEGEditorUpdateContext *update_ctx) {
	for (IDNode *id_node : graph->id_nodes) {
		if (id_node->custom_flags != ID_STATE_MODIFIED) {
			continue;
		}
		DEG_graph_id_type_tag(reinterpret_cast<::Depsgraph *>(graph), GS(id_node->id_orig->name));
		/* TODO(sergey): Do we need to pass original or evaluated ID here? */
		ID *id_orig = id_node->id_orig;
		ID *id_cow = id_node->id_cow;
		/* Gather recalc flags from all changed components. */
		for (ComponentNode *comp_node : id_node->components.values()) {
			if (comp_node->custom_flags != COMPONENT_STATE_DONE) {
				continue;
			}
			DepsNodeFactory *factory = type_get_factory(comp_node->type);
			ROSE_assert(factory != nullptr);
			id_cow->recalc |= factory->id_recalc_tag();
		}

		/**
		 * Inform editors. Only if the data-block is being evaluated a second
		 * time, to distinguish between user edits and initial evaluation when
		 * the data-block becomes visible.
		 *
		 * TODO: image data-blocks do not use COW, so might not be detected
		 * correctly.
		 */
		if (deg_copy_on_write_is_expanded(id_cow)) {
			if (graph->is_active && id_node->is_user_modified) {
				deg_editors_id_update(update_ctx, id_orig);
			}
			/* Inform draw engines that something was changed. */
			flush_engine_data_update(id_cow);
		}
	}
}

void invalidate_tagged_evaluated_data(Depsgraph *graph) {
	(void)graph;
}

}  // namespace

void deg_graph_flush_updates(Depsgraph *graph) {
	/* Sanity checks. */
	ROSE_assert(graph != nullptr);
	Main *main = graph->main;

	graph->time_source->flush_update_tag(graph);

	/* Nothing to update, early out. */
	if (graph->entry_tags.is_empty()) {
		return;
	}
	/* Reset all flags, get ready for the flush. */
	flush_prepare(graph);
	/* Starting from the tagged "entry" nodes, flush outwards. */
	FlushQueue queue;
	flush_schedule_entrypoints(graph, &queue);
	/* Prepare update context for editors. */
	DEGEditorUpdateContext update_ctx;
	update_ctx.main = main;
	update_ctx.depsgraph = (::Depsgraph *)graph;
	update_ctx.scene = graph->scene;
	update_ctx.view_layer = graph->view_layer;
	/* Do actual flush. */
	while (!queue.empty()) {
		OperationNode *op_node = queue.front();
		queue.pop_front();
		while (op_node != nullptr) {
			/* Tag operation as required for update. */
			op_node->flag |= DEPSOP_FLAG_NEEDS_UPDATE;
			/* Inform corresponding ID and component nodes about the change. */
			ComponentNode *comp_node = op_node->owner;
			IDNode *id_node = comp_node->owner;
			flush_handle_id_node(id_node);
			flush_handle_component_node(id_node, comp_node, &queue);
			/* Flush to nodes along links. */
			op_node = flush_schedule_children(op_node, &queue);
		}
	}
	/* Inform editors about all changes. */
	flush_editors_id_update(graph, &update_ctx);
	/* Reset evaluation result tagged which is tagged for update to some state
	 * which is obvious to catch. */
	invalidate_tagged_evaluated_data(graph);
}

void deg_graph_clear_tags(Depsgraph *graph) {
	/* Go over all operation nodes, clearing tags. */
	for (OperationNode *node : graph->operations) {
		node->flag &= ~(DEPSOP_FLAG_DIRECTLY_MODIFIED | DEPSOP_FLAG_NEEDS_UPDATE | DEPSOP_FLAG_USER_MODIFIED);
	}
	/* Clear any entry tags which haven't been flushed. */
	graph->entry_tags.clear();

	graph->time_source->tagged_for_update = false;
}

}  // namespace rose::depsgraph
