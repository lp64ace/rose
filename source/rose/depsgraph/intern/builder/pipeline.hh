#ifndef PIPELINE_HH
#define PIPELINE_HH

#include "intern/depsgraph_types.hh"
#include "intern/builder/deg_builder_cache.hh"

struct Depsgraph;
struct Main;
struct Scene;
struct ViewLayer;

namespace rose::depsgraph {

struct Depsgraph;
class DepsgraphNodeBuilder;
class DepsgraphRelationBuilder;

/**
 * Base class for Depsgraph Builder pipelines.
 * Basically it runs through the following steps:
 * - sanity check
 * - build nodes
 * - build relations
 * - finalize
 */
class AbstractBuilderPipeline {
public:
	AbstractBuilderPipeline(::Depsgraph *graph);
	virtual ~AbstractBuilderPipeline() = default;

	void build();

protected:
	Depsgraph *deg_graph_;
	Main *main_;
	Scene *scene_;
	ViewLayer *view_layer_;
	DepsgraphBuilderCache builder_cache_;

	virtual std::unique_ptr<DepsgraphNodeBuilder> construct_node_builder();
	virtual std::unique_ptr<DepsgraphRelationBuilder> construct_relation_builder();

	virtual void build_step_sanity_check();
	void build_step_nodes();
	void build_step_relations();
	void build_step_finalize();

	virtual void build_nodes(DepsgraphNodeBuilder &node_builder) = 0;
	virtual void build_relations(DepsgraphRelationBuilder &relation_builder) = 0;
};

}  // namespace rose::depsgraph

#endif	// !PIPELINE_HH
