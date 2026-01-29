#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "DNA_collection_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "KER_collection.h"
#include "KER_main.h"
#include "KER_scene.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"

#include "intern/node/deg_node.hh"
#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"

#include "intern/depsgraph.hh"
#include "intern/depsgraph_registry.hh"
#include "intern/depsgraph_relation.hh"
#include "intern/depsgraph_tag.hh"
#include "intern/depsgraph_types.hh"

#include "builder/pipeline_view_layer.hh"

namespace deg = rose::depsgraph;

void DEG_graph_build_from_view_layer(Depsgraph *graph) {
	deg::ViewLayerBuilderPipeline builder(graph);
	builder.build();
}

void DEG_graph_relations_update(Depsgraph *graph) {
	deg::Depsgraph *deg_graph = (deg::Depsgraph *)graph;
	if (!deg_graph->need_update) {
		/* Graph is up to date, nothing to do. */
		return;
	}
	DEG_graph_build_from_view_layer(graph);
}

void DEG_relations_tag_update(Main *main) {
	for (deg::Depsgraph *depsgraph : deg::get_all_registered_graphs(main)) {
		DEG_graph_relations_update(reinterpret_cast<Depsgraph *>(depsgraph));
	}
}
