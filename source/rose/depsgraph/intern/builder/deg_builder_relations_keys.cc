#include "deg_builder_relations.hh"

namespace rose::depsgraph {

TimeSourceKey::TimeSourceKey() : id(nullptr) {
}

TimeSourceKey::TimeSourceKey(ID *id) : id(id) {
}

std::string TimeSourceKey::identifier() const {
	return "TimeSourceKey";
}

ComponentKey::ComponentKey() : id(nullptr), type(NodeType::UNDEFINED), name("") {
}

ComponentKey::ComponentKey(ID *id, NodeType type, const char *name) : id(id), type(type), name(name) {
}

std::string ComponentKey::identifier() const {
	const char *idname = (id) ? id->name : "<None>";
	std::string result = std::string("ComponentKey(");
	result += idname;
	result += ", " + std::string(DEG_node_type_as_string(type));
	if (name[0] != '\0') {
		result += ", '" + std::string(name) + "'";
	}
	result += ')';
	return result;
}

OperationKey::OperationKey() : id(nullptr), component_type(NodeType::UNDEFINED), component_name(""), opcode(OperationCode::OPERATION), name(""), name_tag(-1) {
}

OperationKey::OperationKey(ID *id, NodeType component_type, const char *name, int name_tag) : id(id), component_type(component_type), component_name(""), opcode(OperationCode::OPERATION), name(name), name_tag(name_tag) {
}

OperationKey::OperationKey(ID *id, NodeType component_type, const char *component_name, const char *name, int name_tag) : id(id), component_type(component_type), component_name(component_name), opcode(OperationCode::OPERATION), name(name), name_tag(name_tag) {
}

OperationKey::OperationKey(ID *id, NodeType component_type, OperationCode opcode) : id(id), component_type(component_type), component_name(""), opcode(opcode), name(""), name_tag(-1) {
}

OperationKey::OperationKey(ID *id, NodeType component_type, const char *component_name, OperationCode opcode) : id(id), component_type(component_type), component_name(component_name), opcode(opcode), name(""), name_tag(-1) {
}

OperationKey::OperationKey(ID *id, NodeType component_type, OperationCode opcode, const char *name, int name_tag) : id(id), component_type(component_type), component_name(""), opcode(opcode), name(name), name_tag(name_tag) {
}

OperationKey::OperationKey(ID *id, NodeType component_type, const char *component_name, OperationCode opcode, const char *name, int name_tag) : id(id), component_type(component_type), component_name(component_name), opcode(opcode), name(name), name_tag(name_tag) {
}

std::string OperationKey::identifier() const {
	std::string result = std::string("OperationKey(");
	result += "type: " + std::string(DEG_node_type_as_string(component_type));
	result += ", component name: '" + std::string(component_name) + "'";
	result += ", operation code: " + std::string(DEG_operation_code_as_string(opcode));
	if (name[0] != '\0') {
		result += ", '" + std::string(name) + "'";
	}
	result += ")";
	return result;
}

RNAPathKey::RNAPathKey(ID *id, const char *path, RNAPointerSource source) : id(id), source(source) {
	/* Create ID pointer for root of path lookup. */
	PointerRNA id_ptr = RNA_id_pointer_create(id);
	/* Try to resolve path. */
	if (!RNA_path_resolve_property(&id_ptr, path, &ptr, &prop)) {
		ptr = PointerRNA_NULL;
		prop = nullptr;
	}
}

RNAPathKey::RNAPathKey(ID *id, const PointerRNA &ptr, PropertyRNA *prop, RNAPointerSource source) : id(id), ptr(ptr), prop(prop), source(source) {
}

std::string RNAPathKey::identifier() const {
	const char *id_name = (id) ? id->name : "<No ID>";
	const char *prop_name = (prop) ? RNA_property_identifier(prop) : "<No Prop>";
	return std::string("RnaPathKey(") + "id: " + id_name + ", prop: '" + prop_name + "')";
}

}  // namespace rose::depsgraph
