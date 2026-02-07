#ifndef DEG_NODE_H
#define DEG_NODE_H

#include "DEG_depsgraph.h"

#include "LIB_vector.hh"

#include "intern/depsgraph_relation.hh"
#include "intern/depsgraph_types.hh"

#include <string>

namespace rose::depsgraph {

struct Depsgraph;
struct OperationNode;
struct Relation;

/* Metatype of Nodes - The general "level" in the graph structure
 * the node serves. */
enum class NodeClass {
	/* Types generally unassociated with user-visible entities,
	 * but needed for graph functioning. */
	GENERIC = 0,
	/* [Outer Node] An "aspect" of evaluating/updating an ID-Block, requiring
	 * certain types of evaluation behavior. */
	COMPONENT = 1,
	/* [Inner Node] A glorified function-pointer/callback for scheduling up
	 * evaluation operations for components, subject to relationship
	 * requirements. */
	OPERATION = 2,
};

const char *DEG_node_class_as_string(NodeClass node_class);

enum class NodeType {
	/* Fallback type for invalid return value */
	UNDEFINED = 0,
	/* Inner Node (Operation) */
	OPERATION,

	/* -------------------------------------------------------------------- */
	/** \name Generic Types
	 * \{ */

	/* Time-Source */
	TIMESOURCE,
	/* ID-Block reference - used as landmarks/collection point for components,
	 * but not usually part of main graph. */
	ID_REF,

	/** \} */

	/* -------------------------------------------------------------------- */
	/** \name Outer Types
	 * \{ */

	/* Parameters Component - Default when nothing else fits. */
	PARAMETERS,
	/* Animation Component */
	ANIMATION,
	/* Transform Component (Parenting/Constraints) */
	TRANSFORM,
	/* Geometry Component (#Mesh / #DispList) */
	GEOMETRY,
	/**
	 * Component which contains all operations needed for layer collections
	 * evaluation.
	 */
	LAYER_COLLECTIONS,
	/**
	 * Entry component of majority of ID nodes: prepares CoW pointers for
	 * execution.
	 */
	COPY_ON_WRITE,
	/**
	 * Used by all operations which are updating object when something is
	 * changed in view layer.
	 */
	OBJECT_FROM_LAYER,
	/* Armature Component (#Armature) */
	ARMATURE,

	/**
	 * Component which is used to define visibility relation between IDs, on the ID level.
	 *
	 * Consider two ID nodes NodeA and NodeB, with the relation between visibility components going
	 * as NodeA -> NodeB. If NodeB is considered visible on screen, then the relation will ensure
	 * that NodeA is also visible. The way how relation is oriented could be seen as a inverted from
	 * visibility dependency point of view, but it follows the same direction as data dependency
	 * which simplifies common algorithms which are dealing with relations and visibility.
	 *
	 * The fact that the visibility operates on the ID level basically means that all components in
	 * the NodeA will be considered as affecting directly visible when NodeB's visibility is
	 * affecting directly visible ID.
	 *
	 * This is the way to ensure objects needed for visualization without any actual data dependency
	 * properly evaluated. Example of this is custom shapes for bones.
	 */
	VISIBILITY,

	/** \} */

	/* -------------------------------------------------------------------- */
	/** \name Evaluation-Related Outer Types (with Subdata)
	 * \{ */

	/* Pose Component - Owner/Container of Bones Eval */
	EVAL_POSE,
	/* Bone Component - Child/Subcomponent of Pose */
	BONE,
	/* Duplication system. Used to force duplicated objects visible when
	 * when duplicator is visible. */
	DUPLI,
	/* Synchronization back to original datablock. */
	SYNCHRONIZATION,
	SIMULATION,

	/** \} */

	/* Total number of meaningful node types. */
	NUM_TYPES,
};

const char *DEG_node_type_as_string(NodeType type);

struct Node {
	/* Helper class for static typeinfo in subclasses. */
	struct TypeInfo {
		TypeInfo(NodeType type, const char *type_name, int recalc = 0);

		NodeType type;
		const char *type_name;
		int recalc;
	};

	/* Relationships between nodes
	 * The reason why all depsgraph nodes are descended from this type (apart
	 * from basic serialization benefits - from the typeinfo) is that we can
	 * have relationships between these nodes. */
	typedef rose::Vector<Relation *> Relations;

	std::string name;	/* Identifier - mainly for debugging purposes. */
	NodeType type;		/* Structural type of node. */
	Relations inlinks;	/* Nodes which this one depends on. */
	Relations outlinks; /* Nodes which depend on this one. */

	/**
	 * Generic tags for traversal algorithms and such.
	 *
	 * Actual meaning of values depends on a specific area. Every area is to
	 * clean this before use.
	 */
	int custom_flags;

	Node();
	virtual ~Node();

	/** Generic identifier for Depsgraph Nodes. */
	virtual std::string identifier() const;

	virtual void init(const ID * /*id*/, const char * /*subdata*/) {
	}

	virtual void tag_update(Depsgraph * /*graph*/, eUpdateSource /*source*/) {
	}

	virtual OperationNode *get_entry_operation() {
		return nullptr;
	}
	virtual OperationNode *get_exit_operation() {
		return nullptr;
	}

	virtual NodeClass get_class() const;

	MEM_CXX_CLASS_ALLOC_FUNCS("Node");
};

/* Macros for common static typeinfo. */
#define DEG_DEPSNODE_DECLARE static const Node::TypeInfo typeinfo
#define DEG_DEPSNODE_DEFINE(NodeType, type_, tname_) const Node::TypeInfo NodeType::typeinfo = Node::TypeInfo(type_, tname_)

void deg_register_base_depsnodes();

}  // namespace rose::depsgraph

#endif	// !DEG_NODE_H
