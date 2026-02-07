#include "intern/builder/deg_builder_rna.hh"

#include <cstring>

#include "MEM_guardedalloc.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "DNA_action_types.h"
#include "DNA_armature_types.h"
#include "DNA_object_types.h"

#include "RNA_access.h"
#include "RNA_prototypes.h"

#include "intern/builder/deg_builder.hh"
#include "intern/depsgraph.hh"
#include "intern/node/deg_node.hh"
#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"

namespace rose::depsgraph {

/* ********************************* ID Data ******************************** */

class RNANodeQueryIDData {
public:
	explicit RNANodeQueryIDData(const ID *id) : id_(id) {
	}

	~RNANodeQueryIDData() {
	}

protected:
	/* ID this data corresponds to. */
	const ID *id_;
};

/* ***************************** Node Identifier **************************** */

RNANodeIdentifier::RNANodeIdentifier() : id(nullptr), type(NodeType::UNDEFINED), component_name(""), operation_code(OperationCode::OPERATION), operation_name(), operation_name_tag(-1) {
}

bool RNANodeIdentifier::is_valid() const {
	return id != nullptr && type != NodeType::UNDEFINED;
}

/* ********************************** Query ********************************* */

RNANodeQuery::RNANodeQuery(Depsgraph *depsgraph, DepsgraphBuilder *builder) : depsgraph_(depsgraph), builder_(builder) {
}

RNANodeQuery::~RNANodeQuery() = default;

Node *RNANodeQuery::find_node(const PointerRNA *ptr, const PropertyRNA *prop, RNAPointerSource source) {
	const RNANodeIdentifier node_identifier = construct_node_identifier(ptr, prop, source);
	if (!node_identifier.is_valid()) {
		return nullptr;
	}
	IDNode *id_node = depsgraph_->find_id_node(node_identifier.id);
	if (id_node == nullptr) {
		return nullptr;
	}
	ComponentNode *comp_node = id_node->find_component(node_identifier.type, node_identifier.component_name);
	if (comp_node == nullptr) {
		return nullptr;
	}
	if (node_identifier.operation_code == OperationCode::OPERATION) {
		return comp_node;
	}
	return comp_node->find_operation(node_identifier.operation_code, node_identifier.operation_name, node_identifier.operation_name_tag);
}

bool RNANodeQuery::contains(const char *prop_identifier, const char *rna_path_component) {
	const char *substr = strstr(prop_identifier, rna_path_component);
	if (substr == nullptr) {
		return false;
	}

	/* If substr != prop_identifier, it means that the substring is found further in prop_identifier,
	 * and that thus index -1 is a valid memory location. */
	const bool start_ok = substr == prop_identifier || substr[-1] == '.';
	if (!start_ok) {
		return false;
	}

	const size_t component_len = strlen(rna_path_component);
	const bool end_ok = ELEM(substr[component_len], '\0', '.', '[');
	return end_ok;
}

RNANodeIdentifier RNANodeQuery::construct_node_identifier(const PointerRNA *ptr, const PropertyRNA *prop, RNAPointerSource source) {
	RNANodeIdentifier node_identifier;
	if (ptr->type == nullptr) {
		return node_identifier;
	}
	/* Set default values for returns. */
	node_identifier.id = ptr->owner;
	node_identifier.component_name = "";
	node_identifier.operation_code = OperationCode::OPERATION;
	node_identifier.operation_name = "";
	node_identifier.operation_name_tag = -1;
	/* Handling of commonly known scenarios. */
	if (rna_prop_affects_parameters_node(ptr, prop)) {
		/* Custom properties of bones are placed in their components to improve granularity. */
		if (RNA_struct_is_a(ptr->type, &RNA_PoseBone)) {
			const PoseChannel *pchan = static_cast<const PoseChannel *>(ptr->data);
			node_identifier.type = NodeType::BONE;
			node_identifier.component_name = pchan->name;
		}
		else {
			node_identifier.type = NodeType::PARAMETERS;
		}
		node_identifier.operation_code = OperationCode::ID_PROPERTY;
		node_identifier.operation_name = RNA_property_identifier(reinterpret_cast<const PropertyRNA *>(prop));
		return node_identifier;
	}
	if (ptr->type == &RNA_PoseBone) {
		const PoseChannel *pchan = static_cast<const PoseChannel *>(ptr->data);
		/* Bone - generally, we just want the bone component. */
		node_identifier.type = NodeType::BONE;
		node_identifier.component_name = pchan->name;
		/* However check property name for special handling. */
		if (prop != nullptr) {
			Object *object = reinterpret_cast<Object *>(node_identifier.id);
			const char *prop_name = RNA_property_identifier(prop);
			/* Final transform properties go to the Done node for the exit. */
			if (STREQ(prop_name, "head") || STREQ(prop_name, "tail") || STREQ(prop_name, "length") || STRPREFIX(prop_name, "matrix")) {
				if (source == RNAPointerSource::EXIT) {
					node_identifier.operation_code = OperationCode::BONE_DONE;
				}
			}
			/* And other properties can always go to the entry operation. */
			else {
				node_identifier.operation_code = OperationCode::BONE_LOCAL;
			}
		}
		return node_identifier;
	}
	else if (ptr->type == &RNA_Object) {
		/* Transforms props? */
		if (prop != nullptr) {
			const char *prop_identifier = RNA_property_identifier((PropertyRNA *)prop);
			/* TODO(sergey): How to optimize this? */
			if (contains(prop_identifier, "location") || contains(prop_identifier, "matrix_basis") || contains(prop_identifier, "matrix_channel") || contains(prop_identifier, "matrix_inverse") || contains(prop_identifier, "matrix_local") || contains(prop_identifier, "matrix_parent_inverse") || contains(prop_identifier, "matrix_world") || contains(prop_identifier, "rotation_axis_angle") || contains(prop_identifier, "rotation_euler") || contains(prop_identifier, "rotation_mode") || contains(prop_identifier, "rotation_quaternion") || contains(prop_identifier, "scale") || contains(prop_identifier, "delta_location") || contains(prop_identifier, "delta_rotation_euler") || contains(prop_identifier, "delta_rotation_quaternion") || contains(prop_identifier, "delta_scale")) {
				node_identifier.type = NodeType::TRANSFORM;
				return node_identifier;
			}
			if (contains(prop_identifier, "data")) {
				/* We access object.data, most likely a geometry.
				 * Might be a bone tho. */
				node_identifier.type = NodeType::GEOMETRY;
				return node_identifier;
			}
			if (STREQ(prop_identifier, "hide_viewport") || STREQ(prop_identifier, "hide_render")) {
				node_identifier.type = NodeType::OBJECT_FROM_LAYER;
				return node_identifier;
			}
			if (STREQ(prop_identifier, "dimensions")) {
				node_identifier.type = NodeType::PARAMETERS;
				node_identifier.operation_code = OperationCode::DIMENSIONS;
				return node_identifier;
			}
		}
	}
	if (prop != nullptr) {
		/* All unknown data effectively falls under "parameter evaluation". */
		node_identifier.type = NodeType::PARAMETERS;
		node_identifier.operation_code = OperationCode::PARAMETERS_EVAL;
		node_identifier.operation_name = "";
		node_identifier.operation_name_tag = -1;
		return node_identifier;
	}
	return node_identifier;
}

RNANodeQueryIDData *RNANodeQuery::ensure_id_data(const ID *id) {
	std::unique_ptr<RNANodeQueryIDData> &id_data = id_data_map_.lookup_or_add_cb(id, [&]() {
		return std::make_unique<RNANodeQueryIDData>(id);
	});
	return id_data.get();
}

bool rna_prop_affects_parameters_node(const PointerRNA *ptr, const PropertyRNA *prop) {
	return prop != nullptr && RNA_property_is_idprop(prop);
}

}  // namespace blender::deg
