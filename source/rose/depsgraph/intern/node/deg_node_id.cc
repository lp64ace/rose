#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_factory.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/eval/deg_eval_copy_on_write.hh"

#include "KER_lib_id.h"

namespace rose::depsgraph {

/* clang-format off */

const char *DEG_linked_state_as_string(eDepsNode_LinkedState_Type linked_state) {
	switch (linked_state) {
		case DEG_ID_LINKED_INDIRECTLY: return "INDIRECTLY";
		case DEG_ID_LINKED_VIA_SET: return "VIA_SET";
		case DEG_ID_LINKED_DIRECTLY: return "DIRECTLY";
	}
	ROSE_assert_msg(0, "Unhandled linked state, should never happen.");
	return "UNKNOWN";
}

/* clang-format on */

IDNode::ComponentIDKey::ComponentIDKey(NodeType type, const char *name) : type(type), name(name) {
}

bool IDNode::ComponentIDKey::operator==(const ComponentIDKey &other) const {
	return type == other.type && STREQ(name, other.name);
}

uint64_t IDNode::ComponentIDKey::hash() const {
	const int type_as_int = static_cast<int>(type);
	return LIB_ghashutil_combine_hash(LIB_ghashutil_uinthash(type_as_int), LIB_ghashutil_strhash_p(name));
}

void IDNode::init(const ID *id, const char *subdata) {
	ROSE_assert(id != nullptr);
	/* Store ID-pointer. */
	id_type = GS(id->name);
	id_orig = (ID *)id;
	id_orig_session_uuid = id->uuid;
	eval_flags = 0;
	previous_eval_flags = 0;
	customdata_masks = DEGCustomDataMeshMasks();
	previous_customdata_masks = DEGCustomDataMeshMasks();
	linked_state = DEG_ID_LINKED_INDIRECTLY;
	is_directly_visible = true;
	is_collection_fully_expanded = false;
	has_base = false;
	is_user_modified = false;
	id_cow_recalc_backup = 0;

	visible_components_mask = 0;
	previously_visible_components_mask = 0;
}

void IDNode::init_copy_on_write(ID *id_cow_hint) {
	/* Create pointer as early as possible, so we can use it for function
	 * bindings. Rest of data we'll be copying to the new datablock when
	 * it is actually needed. */
	if (id_cow_hint != nullptr) {
		// BLI_assert(deg_copy_on_write_is_needed(id_orig));
		if (deg_copy_on_write_is_needed(id_orig)) {
			id_cow = id_cow_hint;
		}
		else {
			id_cow = id_orig;
		}
	}
	else if (deg_copy_on_write_is_needed(id_orig)) {
		id_cow = (ID *)KER_libblock_alloc_notest(GS(id_orig->name));
		// fprintf(stdout, "[Depsgraph] Init shallow copy for %s : id_orig 0x%p id_cow 0x%p.\n", id_orig->name, id_orig, id_cow);
		deg_tag_copy_on_write_id(id_cow, id_orig);
	}
	else {
		id_cow = id_orig;
	}
}

IDNode::~IDNode() {
	destroy();
}

void IDNode::destroy() {
	if (id_orig == nullptr) {
		return;
	}

	for (ComponentNode *comp_node : components.values()) {
		delete comp_node;
	}

	/* Free memory used by this CoW ID. */
	if (!ELEM(id_cow, id_orig, nullptr)) {
		// fprintf(stdout, "[Depsgraph] Free shallow copy for %s : id_orig 0x%p id_cow 0x%p.\n", id_orig->name, id_orig, id_cow);
		
		deg_free_copy_on_write_datablock(id_cow);
		MEM_freeN(id_cow);
		id_cow = nullptr;
	}

	/* Tag that the node is freed. */
	id_orig = nullptr;
}

std::string IDNode::identifier() const {
	char orig_ptr[24], cow_ptr[24];
	LIB_strnformat(orig_ptr, sizeof(orig_ptr), "0x%p", id_orig);
	LIB_strnformat(cow_ptr, sizeof(cow_ptr), "0x%p", id_cow);
	return std::string(DEG_node_type_as_string(type)) + " : " + name + " { orig : " + orig_ptr + ", eval : " + cow_ptr + ", visible : " + (is_directly_visible ? "true" : "false") + "}";
}

ComponentNode *IDNode::find_component(NodeType type, const char *name) const {
	ComponentIDKey key(type, name);
	return components.lookup_default(key, nullptr);
}

ComponentNode *IDNode::add_component(NodeType type, const char *name) {
	ComponentNode *comp_node = find_component(type, name);
	if (!comp_node) {
		DepsNodeFactory *factory = type_get_factory(type);
		comp_node = (ComponentNode *)factory->create_node(this->id_orig, "", name);

		/* Register. */
		ComponentIDKey key(type, name);
		components.add_new(key, comp_node);
		comp_node->owner = this;
	}
	return comp_node;
}

void IDNode::tag_update(Depsgraph *graph, eUpdateSource source) {
	for (ComponentNode *comp_node : components.values()) {
		/* Relations update does explicit animation update when needed. Here we ignore animation
		 * component to avoid loss of possible unkeyed changes. */
		if (comp_node->type == NodeType::ANIMATION && (source & DEG_UPDATE_SOURCE_RELATIONS)) {
			continue;
		}
		comp_node->tag_update(graph, source);
	}
}

void IDNode::finalize_build(Depsgraph *graph) {
	/* Finalize build of all components. */
	for (ComponentNode *comp_node : components.values()) {
		comp_node->finalize_build(graph);
	}
	visible_components_mask = get_visible_components_mask();
}

IDComponentsMask IDNode::get_visible_components_mask() const {
	IDComponentsMask result = 0;
	for (ComponentNode *comp_node : components.values()) {
		if (comp_node->affects_directly_visible) {
			const int component_type_as_int = static_cast<int>(comp_node->type);
			ROSE_assert(component_type_as_int < 64);
			result |= (1ULL << component_type_as_int);
		}
	}
	return result;
}

}  // namespace rose::depsgraph
