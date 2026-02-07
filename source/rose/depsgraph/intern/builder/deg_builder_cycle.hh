#ifndef DEG_BUILDER_CYCLE_HH
#define DEG_BUILDER_CYCLE_HH

namespace rose::depsgraph {

struct Depsgraph;

/* Detect and solve dependency cycles. */
void deg_graph_detect_cycles(Depsgraph *graph);

}  // namespace rose::depsgraph

#endif	// !DEG_BUILDER_CYCLE_HH
