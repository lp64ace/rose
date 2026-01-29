#ifndef DEG_BUILDER_REMOVE_NOOP_HH
#define DEG_BUILDER_REMOVE_NOOP_HH

namespace rose::depsgraph {

struct Depsgraph;

/* Remove all no-op nodes that have zero outgoing relations. */
void deg_graph_remove_unused_noops(Depsgraph *graph);

}  // namespace rose::depsgraph

#endif	// !DEG_BUILDER_REMOVE_NOOP_HH
