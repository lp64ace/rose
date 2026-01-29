#include "pipeline.hh"

#include "KER_global.h"

#include "DNA_scene_types.h"

#include "deg_builder.hh"
#include "deg_builder_cycle.hh"
#include "deg_builder_nodes.hh"
#include "deg_builder_relations.hh"

namespace rose::depsgraph {

AbstractBuilderPipeline::AbstractBuilderPipeline(::Depsgraph *graph) : deg_graph_(reinterpret_cast<Depsgraph *>(graph)), main_(deg_graph_->main), scene_(deg_graph_->scene), view_layer_(deg_graph_->view_layer) {
}

void AbstractBuilderPipeline::build() {
	build_step_sanity_check();
	build_step_nodes();
	build_step_relations();
	build_step_finalize();
}

void AbstractBuilderPipeline::build_step_sanity_check() {
	ROSE_assert(LIB_haslink(&scene_->view_layers, view_layer_));
	ROSE_assert(deg_graph_->scene == scene_);
	ROSE_assert(deg_graph_->view_layer == view_layer_);
}

void AbstractBuilderPipeline::build_step_nodes() {
	/* Generate all the nodes in the graph first */
	std::unique_ptr<DepsgraphNodeBuilder> node_builder = construct_node_builder();
	node_builder->begin_build();
	build_nodes(*node_builder);
	node_builder->end_build();
}

void AbstractBuilderPipeline::build_step_relations() {
	/* Hook up relationships between operations - to determine evaluation order. */
	std::unique_ptr<DepsgraphRelationBuilder> relation_builder = construct_relation_builder();
	relation_builder->begin_build();
	build_relations(*relation_builder);
	relation_builder->build_copy_on_write_relations();
}

void AbstractBuilderPipeline::build_step_finalize() {
	/* Detect and solve cycles. */
	deg_graph_detect_cycles(deg_graph_);
	/* Store pointers to commonly used evaluated datablocks. */
	deg_graph_->scene_cow = (Scene *)deg_graph_->get_cow_id(&deg_graph_->scene->id);
	/* Flush visibility layer and re-schedule nodes for update. */
	deg_graph_build_finalize(main_, deg_graph_);
	DEG_graph_tag_on_visible_update(reinterpret_cast<::Depsgraph *>(deg_graph_), false);
	/* Relations are up to date. */
	deg_graph_->need_update = false;
}

std::unique_ptr<DepsgraphNodeBuilder> AbstractBuilderPipeline::construct_node_builder() {
	return std::make_unique<DepsgraphNodeBuilder>(main_, deg_graph_, &builder_cache_);
}

std::unique_ptr<DepsgraphRelationBuilder> AbstractBuilderPipeline::construct_relation_builder() {
	return std::make_unique<DepsgraphRelationBuilder>(main_, deg_graph_, &builder_cache_);
}

}  // namespace rose::depsgraph
