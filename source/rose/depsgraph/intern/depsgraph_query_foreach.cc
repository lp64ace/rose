#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

#include "KER_idtype.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_scene.h"

#include "LIB_thread.h"

#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_factory.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"
#include "intern/node/deg_node_time.hh"

#include "intern/depsgraph_registry.hh"

#include "depsgraph.hh"

#include <deque>

namespace deg = rose::depsgraph;

namespace rose::depsgraph {
namespace {

using TraversalQueue = std::deque<OperationNode *>;

using DEGForeachOperation = void (*)(OperationNode *, void *);

bool deg_foreach_needs_visit(const OperationNode *op_node, const int flags) {
	if (flags & DEG_FOREACH_COMPONENT_IGNORE_TRANSFORM_SOLVERS) {
		if (op_node->opcode == OperationCode::RIGIDBODY_SIM) {
			return false;
		}
	}
	return true;
}

void deg_foreach_dependent_operation(const Depsgraph *graph, const IDNode *target_id_node, eDepsObjectComponentType source_component_type, int flags, DEGForeachOperation callback, void *user_data) {
	if (target_id_node == nullptr) {
		/**
		 * Shall we inform or assert here about attempt to start
		 * iterating over non-existing ID?
		 */
		return;
	}
	/* Start with scheduling all operations from ID node. */
	TraversalQueue queue;
	Set<OperationNode *> scheduled;
	for (ComponentNode *comp_node : target_id_node->components.values()) {
		if (comp_node->type == NodeType::VISIBILITY) {
      		/* Visibility component is only used internally. It is not to be reporting dependencies to the outer world. */
			continue;
    	}

		if (source_component_type != DEG_OB_COMP_ANY && DEG_node_type_to_object_component(comp_node->type) != source_component_type) {
			continue;
		}
		for (OperationNode *op_node : comp_node->operations) {
			if (!deg_foreach_needs_visit(op_node, flags)) {
				continue;
			}
			queue.push_back(op_node);
			scheduled.add(op_node);
    	}
	}
	/* Process the queue. */
	while (!queue.empty()) {
		/* get next operation node to process. */
		OperationNode *op_node = queue.front();
		queue.pop_front();
		for (;;) {
			callback(op_node, user_data);
			/* Schedule outgoing operation nodes. */
			if (op_node->outlinks.size() == 1) {
				OperationNode *to_node = (OperationNode *)op_node->outlinks[0]->to;
				if (!scheduled.contains(to_node) && deg_foreach_needs_visit(to_node, flags)) {
					scheduled.add_new(to_node);
					op_node = to_node;
				}
				else {
					break;
				}
			}
			else {
				for (Relation *rel : op_node->outlinks) {
					OperationNode *to_node = (OperationNode *)rel->to;
					if (!scheduled.contains(to_node) && deg_foreach_needs_visit(to_node, flags)) {
						queue.push_front(to_node);
						scheduled.add_new(to_node);
					}
				}
				break;
			}
		}
	}
}

struct ForeachIDComponentData {
	DEGForeachIDComponentCallback callback;
	void *user_data;
	IDNode *target_id_node;
	Set<ComponentNode *> visited;
};


void deg_foreach_dependent_component_callback(OperationNode *op_node, void *user_data_v) {
	ForeachIDComponentData *user_data = reinterpret_cast<ForeachIDComponentData *>(user_data_v);
	ComponentNode *comp_node = op_node->owner;
	IDNode *id_node = comp_node->owner;
	if (id_node != user_data->target_id_node && !user_data->visited.contains(comp_node)) {
    	user_data->callback(id_node->id_orig, DEG_node_type_to_object_component(comp_node->type), user_data->user_data);
		user_data->visited.add_new(comp_node);
	}
}

void deg_foreach_dependent_ID_component(const Depsgraph *graph, const ID *id, eDepsObjectComponentType source_component_type, int flags, DEGForeachIDComponentCallback callback, void *user_data) {
	ForeachIDComponentData data;
	data.callback = callback;
	data.user_data = user_data;
	data.target_id_node = graph->find_id_node(id);
	deg_foreach_dependent_operation(graph, data.target_id_node, source_component_type, flags, deg_foreach_dependent_component_callback, &data);
}

}
}	//	namespace rose::despgraph

/* -------------------------------------------------------------------- */
/** \name DEG traversal
 * \{ */

void DEG_foreach_dependent_ID_component(const Depsgraph *depsgraph, const ID *id, eDepsObjectComponentType source, int flags, DEGForeachIDComponentCallback callback, void *user_data) {
	deg::deg_foreach_dependent_ID_component((const deg::Depsgraph *)depsgraph, id, source, flags, callback, user_data);
}

/** \} */
