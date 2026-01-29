#ifndef DEG_BUILDER_RELATIONS_IMPL_HH
#define DEG_BUILDER_RELATIONS_IMPL_HH

#include "deg_builder_relations.hh"

namespace rose::depsgraph {

template<typename KeyType> OperationNode *DepsgraphRelationBuilder::find_operation_node(const KeyType &key) {
	Node *node = get_node(key);
	return node != nullptr ? node->get_exit_operation() : nullptr;
}

template<typename KeyFrom, typename KeyTo> Relation *DepsgraphRelationBuilder::add_relation(const KeyFrom &key_from, const KeyTo &key_to, const char *description, int flags) {
	Node *node_from = get_node(key_from);
	Node *node_to = get_node(key_to);
	OperationNode *op_from = node_from ? node_from->get_exit_operation() : nullptr;
	OperationNode *op_to = node_to ? node_to->get_entry_operation() : nullptr;
	if (op_from && op_to) {
		return add_operation_relation(op_from, op_to, description, flags);
	}
	else {
		if (!op_from) {
			/* XXX TODO: handle as error or report if needed. */
			fprintf(stderr, "[Depsrgaph] add_relation(%s) - Could not find op_from (%s)\n", description, key_from.identifier().c_str());
		}
		else {
			fprintf(stderr, "add_relation(%s) - Failed, but op_from (%s) was ok\n", description, key_from.identifier().c_str());
		}
		if (!op_to) {
			/* XXX TODO: handle as error or report if needed. */
			fprintf(stderr, "[Depsrgaph] add_relation(%s) - Could not find op_to (%s)\n", description, key_to.identifier().c_str());
		}
		else {
			fprintf(stderr, "[Depsrgaph] add_relation(%s) - Failed, but op_to (%s) was ok\n", description, key_to.identifier().c_str());
		}
	}
	return nullptr;
}

template<typename KeyTo> Relation *DepsgraphRelationBuilder::add_relation(const TimeSourceKey &key_from, const KeyTo &key_to, const char *description, int flags) {
	TimeSourceNode *time_from = get_node(key_from);
	Node *node_to = get_node(key_to);
	OperationNode *op_to = node_to ? node_to->get_entry_operation() : nullptr;
	if (time_from != nullptr && op_to != nullptr) {
		return add_time_relation(time_from, op_to, description, flags);
	}
	return nullptr;
}

template<typename KeyType> Relation *DepsgraphRelationBuilder::add_node_handle_relation(const KeyType &key_from, const DepsNodeHandle *handle, const char *description, int flags) {
	Node *node_from = get_node(key_from);
	OperationNode *op_from = node_from ? node_from->get_exit_operation() : nullptr;
	OperationNode *op_to = handle->node->get_entry_operation();
	if (op_from != nullptr && op_to != nullptr) {
		return add_operation_relation(op_from, op_to, description, flags);
	}
	else {
		if (!op_from) {
			fprintf(stderr, "add_node_handle_relation(%s) - Could not find op_from (%s)\n", description, key_from.identifier().c_str());
		}
		if (!op_to) {
			fprintf(stderr, "add_node_handle_relation(%s) - Could not find op_to (%s)\n", description, key_from.identifier().c_str());
		}
	}
	return nullptr;
}

template<typename KeyTo> Relation *DepsgraphRelationBuilder::add_depends_on_transform_relation(ID *id, const KeyTo &key_to, const char *description, int flags) {
	ComponentKey transform_key(id, NodeType::TRANSFORM);
	return add_relation(transform_key, key_to, description, flags);
}

}  // namespace rose::depsgraph

#endif	// !DEG_BUILDER_RELATIONS_IMPL_HH
