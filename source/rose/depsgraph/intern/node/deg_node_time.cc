#include "deg_node_time.hh"

namespace rose::depsgraph {

void TimeSourceNode::tag_update(Depsgraph *graph, eUpdateSource source) {
	this->tagged_for_update = true;
}

void TimeSourceNode::flush_update_tag(Depsgraph *graph) {
	if (!this->tagged_for_update) {
		return;
	}
	for (Relation *rel : this->outlinks) {
		Node *node = rel->to;
		node->tag_update(graph, DEG_UPDATE_SOURCE_TIME);
	}
}

}  // namespace rose::depsgraph
