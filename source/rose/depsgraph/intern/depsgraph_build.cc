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

void DEG_graph_tag_relations_update(Depsgraph *graph) {
	deg::Depsgraph *deg_graph = reinterpret_cast<deg::Depsgraph *>(graph);
	deg_graph->need_update = true;

	/* NOTE: When relations are updated, it's quite possible that we've got new bases in the scene.
	 * This means, we need to re-create flat array of bases in view layer. */
	/* TODO(sergey): It is expected that bases manipulation tags scene for update to tag bases array
	 * for re-creation. Once it is ensured to happen from all places this implicit tag can be
	 * removed. */
	deg::IDNode *id_node = deg_graph->find_id_node(&deg_graph->scene->id);
	if (id_node != nullptr) {
		graph_id_tag_update(deg_graph->main, deg_graph, &deg_graph->scene->id, 0, deg::DEG_UPDATE_SOURCE_RELATIONS);
	}
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
		DEG_graph_tag_relations_update(reinterpret_cast<Depsgraph *>(depsgraph));
	}
}
