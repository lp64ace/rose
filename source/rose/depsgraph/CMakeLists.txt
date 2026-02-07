# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC .
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories

set(INC_SYS
	# External System Include Directories
	${TBB_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	DEG_depsgraph.h
	DEG_depsgraph_build.h
	DEG_depsgraph_query.h

	intern/builder/deg_builder.cc
	intern/builder/deg_builder.hh
	intern/builder/deg_builder_cache.cc
	intern/builder/deg_builder_cache.hh
	intern/builder/deg_builder_cycle.cc
	intern/builder/deg_builder_cycle.hh
	intern/builder/deg_builder_map.cc
	intern/builder/deg_builder_map.hh
	intern/builder/deg_builder_nodes.cc
	intern/builder/deg_builder_nodes.hh
	intern/builder/deg_builder_relations.cc
	intern/builder/deg_builder_relations.hh
	intern/builder/deg_builder_relations_impl.hh
	intern/builder/deg_builder_relations_keys.cc
	intern/builder/deg_builder_rna.cc
	intern/builder/deg_builder_rna.hh
	intern/builder/pipeline.cc
	intern/builder/pipeline.hh
	intern/builder/deg_builder_remove_noop.cc
	intern/builder/deg_builder_remove_noop.hh
	intern/builder/pipeline_view_layer.cc
	intern/builder/pipeline_view_layer.hh
	
	intern/eval/deg_eval.cc
	intern/eval/deg_eval.hh
	intern/eval/deg_eval_copy_on_write.cc
	intern/eval/deg_eval_copy_on_write.hh
	intern/eval/deg_eval_flush.cc
	intern/eval/deg_eval_flush.hh
	intern/eval/deg_eval_runtime_backup.cc
	intern/eval/deg_eval_runtime_backup.hh
	intern/eval/deg_eval_runtime_backup_modifier.cc
	intern/eval/deg_eval_runtime_backup_modifier.hh
	intern/eval/deg_eval_runtime_backup_object.cc
	intern/eval/deg_eval_runtime_backup_object.hh
	intern/eval/deg_eval_runtime_backup_scene.cc
	intern/eval/deg_eval_runtime_backup_scene.hh

	intern/node/deg_node.cc
	intern/node/deg_node.hh
	intern/node/deg_node_component.cc
	intern/node/deg_node_component.hh
	intern/node/deg_node_factory.cc
	intern/node/deg_node_factory.hh
	intern/node/deg_node_id.cc
	intern/node/deg_node_id.hh
	intern/node/deg_node_operation.cc
	intern/node/deg_node_operation.hh
	intern/node/deg_node_time.cc
	intern/node/deg_node_time.hh

	intern/depsgraph.cc
	intern/depsgraph.hh
	intern/depsgraph_build.cc
	intern/depsgraph_eval.cc
	intern/depsgraph_query.cc
	intern/depsgraph_registry.cc
	intern/depsgraph_registry.hh
	intern/depsgraph_relation.cc
	intern/depsgraph_relation.hh
	intern/depsgraph_tag.cc
	intern/depsgraph_tag.hh
	intern/depsgraph_types.cc
	intern/depsgraph_types.hh
	intern/depsgraph_update.cc
	intern/depsgraph_update.hh
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	PUBLIC rose::intern::atomic
	PUBLIC rose::source::gpu
	PUBLIC rose::source::dna
	PUBLIC rose::source::rna
	PUBLIC rose::source::roselib
	PUBLIC rose::source::rosekernel
	PUBLIC rose::source::windowmanager
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(depsgraph "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::depsgraph ALIAS depsgraph)
