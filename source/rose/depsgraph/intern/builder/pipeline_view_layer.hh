#ifndef PIPELINE_VIEW_LAYER_HH
#define PIPELINE_VIEW_LAYER_HH

#include "pipeline.hh"

namespace rose::depsgraph {

class ViewLayerBuilderPipeline : public AbstractBuilderPipeline {
public:
	ViewLayerBuilderPipeline(::Depsgraph *graph);

protected:
	virtual void build_nodes(DepsgraphNodeBuilder &node_builder) override;
	virtual void build_relations(DepsgraphRelationBuilder &relation_builder) override;
};

}  // namespace rose::depsgraph

#endif	// !PIPELINE_VIEW_LAYER_HH
