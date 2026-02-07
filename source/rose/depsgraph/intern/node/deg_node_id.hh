#ifndef DEG_NODE_ID_HH
#define DEG_NODE_ID_HH

#include "LIB_ghash.h"
#include "LIB_map.hh"
#include "LIB_utildefines.h"

#include "DNA_ID.h"

#include "deg_node.hh"

namespace rose::depsgraph {

struct ComponentNode;

typedef uint64_t IDComponentsMask;

/* NOTE: We use max comparison to mark an id node that is linked more than once
 * So keep this enum ordered accordingly. */
enum eDepsNode_LinkedState_Type {
	/* Generic indirectly linked id node. */
	DEG_ID_LINKED_INDIRECTLY = 0,
	/* Id node present in the set (background) only. */
	DEG_ID_LINKED_VIA_SET = 1,
	/* Id node directly linked via the SceneLayer. */
	DEG_ID_LINKED_DIRECTLY = 2,
};
const char *DEG_linked_state_as_string(eDepsNode_LinkedState_Type linked_state);

/* ID-Block Reference */
struct IDNode : public Node {
	struct ComponentIDKey {
		ComponentIDKey(NodeType type, const char *name = "");
		uint64_t hash() const;
		bool operator==(const ComponentIDKey &other) const;

		NodeType type;
		const char *name;
	};

	/** Initialize 'id' node - from pointer data given. */
	virtual void init(const ID *id, const char *subdata) override;
	void init_copy_on_write(ID *id_cow_hint = nullptr);
	~IDNode();
	void destroy();

	virtual std::string identifier() const override;

	ComponentNode *find_component(NodeType type, const char *name = "") const;
	ComponentNode *add_component(NodeType type, const char *name = "");

	virtual void tag_update(Depsgraph *graph, eUpdateSource source) override;

	void finalize_build(Depsgraph *graph);

	IDComponentsMask get_visible_components_mask() const;

	/* Type of the ID stored separately, so it's possible to perform check whether CoW is needed
	 * without de-referencing the id_cow (which is not safe when ID is NOT covered by CoW and has
	 * been deleted from the main database.) */
	ID_Type id_type;

	/* ID Block referenced. */
	ID *id_orig;

	/**
	 * Session-wide UUID of the id_orig.
	 * Is used on relations update to map evaluated state from old nodes to the new ones, without
	 * relying on pointers (which are not guaranteed to be unique) and without dereferencing id_orig
	 * which could be "stale" pointer.
	 */
	uint id_orig_session_uuid;

	/**
	 * Evaluated data-block.
	 * Will be covered by the copy-on-write system if the ID Type needs it.
	 */
	ID *id_cow;

	/* Hash to make it faster to look up components. */
	Map<ComponentIDKey, ComponentNode *> components;

	/**
	 * Additional flags needed for scene evaluation.
	 */
	uint32_t eval_flags;
	uint32_t previous_eval_flags;

	/* Extra customdata mask which needs to be evaluated for the mesh object. */
	DEGCustomDataMeshMasks customdata_masks;
	DEGCustomDataMeshMasks previous_customdata_masks;

	eDepsNode_LinkedState_Type linked_state;

	/* Indicates the data-block is visible in the evaluated scene. */
	bool is_directly_visible;

	/* For the collection type of ID, denotes whether collection was fully
	 * recursed into. */
	bool is_collection_fully_expanded;

	/* Is used to figure out whether object came to the dependency graph via a base. */
	bool has_base;

	/* Accumulated flag from operation. Is initialized and used during updates flush. */
	bool is_user_modified;

	/* Copy-on-Write component has been explicitly tagged for update. */
	bool is_cow_explicitly_tagged;

	/* Accumulate recalc flags from multiple update passes. */
	int id_cow_recalc_backup;

	IDComponentsMask visible_components_mask;
	IDComponentsMask previously_visible_components_mask;

	DEG_DEPSNODE_DECLARE;
};

}  // namespace rose::depsgraph

#endif	// !DEG_NODE_ID_HH
