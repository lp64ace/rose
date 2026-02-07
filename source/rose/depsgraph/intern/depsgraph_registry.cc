#include "depsgraph_registry.hh"
#include "depsgraph_types.hh"

#include "KER_main.h"

#include "LIB_utildefines.h"
#include "LIB_vector_set.hh"

#include "depsgraph.hh"

namespace rose::depsgraph {

using GraphRegistry = Map<Main *, VectorSet<Depsgraph *>>;
static GraphRegistry &get_graph_registry() {
	static GraphRegistry graph_registry;
	return graph_registry;
}

void register_graph(Depsgraph *depsgraph) {
	Main *main = depsgraph->main;
	get_graph_registry().lookup_or_add_default(main).add_new(depsgraph);
}

void unregister_graph(Depsgraph *depsgraph) {
	Main *main = depsgraph->main;
	GraphRegistry &graph_registry = get_graph_registry();
	VectorSet<Depsgraph *> &graphs = graph_registry.lookup(main);
	graphs.remove(depsgraph);

	/* If this was the last depsgraph associated with the main, remove the main entry as well. */
	if (graphs.is_empty()) {
		graph_registry.remove(main);
	}
}

Span<Depsgraph *> get_all_registered_graphs(Main *main) {
	VectorSet<Depsgraph *> *graphs = get_graph_registry().lookup_ptr(main);
	if (graphs != nullptr) {
		return *graphs;
	}
	return {};
}

}  // namespace rose::depsgraph
