#ifndef DEG_NODE_TIME_HH
#define DEG_NODE_TIME_HH

#include "deg_node.hh"

namespace rose::depsgraph {

struct TimeSourceNode : public Node {
	virtual void tag_update(Depsgraph *graph, eUpdateSource source) override;
	void flush_update_tag(Depsgraph *graph);

	bool tagged_for_update = false;

	DEG_DEPSNODE_DECLARE;
};

}  // namespace rose::depsgraph

#endif	// !DEG_NODE_TIME_HH
