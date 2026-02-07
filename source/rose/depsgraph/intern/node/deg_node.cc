#include "deg_node.hh"

#include "LIB_utildefines.h"

#include "intern/depsgraph.hh"
#include "intern/depsgraph_relation.hh"

#include "deg_node_factory.hh"
#include "deg_node_time.hh"
#include "deg_node_id.hh"

namespace rose::depsgraph {

/* clang-format off */

const char *DEG_node_class_as_string(NodeClass node_class) {
	switch (node_class) {
		case NodeClass::GENERIC: return "GENERIC";
		case NodeClass::COMPONENT: return "COMPONENT";
		case NodeClass::OPERATION: return "OPERATION";
	}
	ROSE_assert_msg(0, "Unhandled node class, should never happen.");
	return "UNKNOWN";
}

const char *DEG_node_type_as_string(NodeType type) {
	switch (type) {
		case NodeType::UNDEFINED: return "UNDEFINED";
		case NodeType::OPERATION: return "OPERATION";
		/* **** Generic Types **** */
		case NodeType::TIMESOURCE: return "TIMESOURCE";
		case NodeType::ID_REF: return "ID_REF";
        /* **** Outer Types **** */
        case NodeType::PARAMETERS: return "PARAMETERS";
        case NodeType::ANIMATION: return "ANIMATION";
        case NodeType::TRANSFORM: return "TRANSFORM";
        case NodeType::GEOMETRY: return "GEOMETRY";
        case NodeType::LAYER_COLLECTIONS: return "LAYER_COLLECTIONS";
        case NodeType::COPY_ON_WRITE: return "COPY_ON_WRITE";
        case NodeType::OBJECT_FROM_LAYER: return "OBJECT_FROM_LAYER";
        /* **** Evaluation-Related Outer Types (with Subdata) **** */
        case NodeType::EVAL_POSE: return "EVAL_POSE";
        case NodeType::BONE: return "BONE";
        case NodeType::SYNCHRONIZATION: return "SYNCHRONIZATION";
        case NodeType::ARMATURE: return "ARMATURE";
        case NodeType::DUPLI: return "DUPLI";
        case NodeType::VISIBILITY: return "VISIBILITY";
        case NodeType::SIMULATION: return "SIMULATION";
        /* Total number of meaningful node types. */
        case NodeType::NUM_TYPES: return "SpecialCase";
    }
    ROSE_assert_msg(0, "Unhandled node type, should never happen.");
    return "UNKNOWN";
}

/* clang-format on */

/* -------------------------------------------------------------------- */
/** \name Type information.
 * \{ */

Node::TypeInfo::TypeInfo(NodeType type, const char *type_name, int recalc) : type(type), type_name(type_name), recalc(recalc) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Node.
 * \{ */

Node::Node() {
	name = "";
}

Node::~Node() {
	/* Free links. */
	/* NOTE: We only free incoming links. This is to avoid double-free of links
	 * when we're trying to free same link from both its sides. We don't have
	 * dangling links so this is not a problem from memory leaks point of view. */
	for (Relation *rel : inlinks) {
		delete rel;
	}
}

std::string Node::identifier() const {
	return static_cast<std::string>(DEG_node_type_as_string(this->type)) + " : " + name;
}

NodeClass Node::get_class() const {
	if (type == NodeType::OPERATION) {
		return NodeClass::OPERATION;
	}
	if (type < NodeType::PARAMETERS) {
		return NodeClass::GENERIC;
	}

	return NodeClass::COMPONENT;
}

/** \} */

DEG_DEPSNODE_DEFINE(TimeSourceNode, NodeType::TIMESOURCE, "Time Source");
static DepsNodeFactoryImpl<TimeSourceNode> DNTI_TIMESOURCE;

DEG_DEPSNODE_DEFINE(IDNode, NodeType::ID_REF, "ID Node");
static DepsNodeFactoryImpl<IDNode> DNTI_ID_REF;

void deg_register_base_depsnodes() {
	register_node_typeinfo(&DNTI_TIMESOURCE);
	register_node_typeinfo(&DNTI_ID_REF);
}

}  // namespace rose::depsgraph
