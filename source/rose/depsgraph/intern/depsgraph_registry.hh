#ifndef DEGPSGRAPH_REGISTRY_HH
#define DEGPSGRAPH_REGISTRY_HH

#include "LIB_span.hh"

struct Main;

namespace rose::depsgraph {

struct Depsgraph;

void register_graph(Depsgraph *depsgraph);
void unregister_graph(Depsgraph *depsgraph);
Span<Depsgraph *> get_all_registered_graphs(Main *main);

}  // namespace rose::depsgraph

#endif	// !DEGPSGRAPH_REGISTRY_HH
