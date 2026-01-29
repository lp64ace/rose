#ifndef DEG_EVAL_FLUSH_HH
#define DEG_EVAL_FLUSH_HH

namespace rose::depsgraph {

struct Depsgraph;

/**
 * Flush updates from tagged nodes outwards until all affected nodes are tagged.
 */
void deg_graph_flush_updates(struct Depsgraph *graph);

/**
 * Clear tags from all operation nodes.
 */
void deg_graph_clear_tags(struct Depsgraph *graph);

}

#endif	// !DEG_EVAL_FLUSH_HH