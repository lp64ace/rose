#include "pipeline_view_layer.hh"

#include "intern/builder/deg_builder_nodes.hh"
#include "intern/builder/deg_builder_relations.hh"
#include "intern/depsgraph.hh"

namespace rose::depsgraph {

ViewLayerBuilderPipeline::ViewLayerBuilderPipeline(::Depsgraph *graph) : AbstractBuilderPipeline(graph) {
}

void ViewLayerBuilderPipeline::build_nodes(DepsgraphNodeBuilder &node_builder) {
	node_builder.build_view_layer(scene_, view_layer_, DEG_ID_LINKED_DIRECTLY);
}

void ViewLayerBuilderPipeline::build_relations(DepsgraphRelationBuilder &relation_builder) {
	relation_builder.build_view_layer(scene_, view_layer_, DEG_ID_LINKED_DIRECTLY);
}

}  // namespace blender::deg
