#ifndef DEG_BUILDER_NODES_HH
#define DEG_BUILDER_NODES_HH

#include "KER_collection.h"

#include "DEG_depsgraph.h"

#include "intern/builder/deg_builder.hh"
#include "intern/builder/deg_builder_map.hh"
#include "intern/depsgraph_types.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"
#include "intern/node/deg_node_time.hh"

struct Action;
struct Armature;

namespace rose::depsgraph {

class DepsgraphNodeBuilder : public DepsgraphBuilder {
public:
	DepsgraphNodeBuilder(Main *main, Depsgraph *depsgraph, DepsgraphBuilderCache *cache);
	~DepsgraphNodeBuilder();

	/** For given original ID get ID which is created by CoW system. */
	ID *get_cow_id(const ID *id_original) const;
	/** Similar to above, but for the cases when there is no ID node we create one. */
	ID *ensure_cow_id(ID *id_original);

	/* Helper wrapper function which wraps get_cow_id with a needed type cast. */
	template<typename T> T *get_cow_datablock(const T *orig) const {
		return (T *)get_cow_id(&orig->id);
	}

	/* For a given COW datablock get corresponding original one. */
	template<typename T> T *get_orig_datablock(const T *cow) const {
		return (T *)cow->id.orig_id;
	}

	virtual void begin_build();
	virtual void end_build();

	/**
	 * `id_cow_self` is the user of `id_pointer`,
	 * see also `LibraryIDLinkCallbackData` struct definition.
	 */
	int foreach_id_cow_detect_need_for_update_callback(ID *id_cow_self, ID *id_pointer);

	IDNode *add_id_node(ID *id);
	IDNode *find_id_node(ID *id);
	TimeSourceNode *add_time_source();

	ComponentNode *add_component_node(ID *id, NodeType comp_type, const char *comp_name = "");

	OperationNode *add_operation_node(ComponentNode *comp_node, OperationCode opcode, const DepsEvalOperationCb &op = nullptr, const char *name = "", int name_tag = -1);
	OperationNode *add_operation_node(ID *id, NodeType comp_type, const char *comp_name, OperationCode opcode, const DepsEvalOperationCb &op = nullptr, const char *name = "", int name_tag = -1);
	OperationNode *add_operation_node(ID *id, NodeType comp_type, OperationCode opcode, const DepsEvalOperationCb &op = nullptr, const char *name = "", int name_tag = -1);

	OperationNode *ensure_operation_node(ID *id, NodeType comp_type, const char *comp_name, OperationCode opcode, const DepsEvalOperationCb &op = nullptr, const char *name = "", int name_tag = -1);
	OperationNode *ensure_operation_node(ID *id, NodeType comp_type, OperationCode opcode, const DepsEvalOperationCb &op = nullptr, const char *name = "", int name_tag = -1);

	bool has_operation_node(ID *id, NodeType comp_type, const char *comp_name, OperationCode opcode, const char *name = "", int name_tag = -1);

	OperationNode *find_operation_node(ID *id, NodeType comp_type, const char *comp_name, OperationCode opcode, const char *name = "", int name_tag = -1);
	OperationNode *find_operation_node(ID *id, NodeType comp_type, OperationCode opcode, const char *name = "", int name_tag = -1);

	/* Per-ID information about what was already in the dependency graph.
	 * Allows to re-use certain values, to speed up following evaluation. */
	struct IDInfo {
		/* Copy-on-written pointer of the corresponding ID. */
		ID *id_cow;
		/* Mask of visible components from previous state of the
		 * dependency graph. */
		IDComponentsMask previously_visible_components_mask;
		/* Special evaluation flag mask from the previous depsgraph. */
		uint32_t previous_eval_flags;
		/* Mesh CustomData mask from the previous depsgraph. */
		DEGCustomDataMeshMasks previous_customdata_masks;
	};

public:
	virtual void build_id(ID *id);
	
	/* Build function for ID types that do not need their own build_xxx() function. */
	virtual void build_generic_id(ID *id);
	virtual void build_idproperties(IDProperty *id_property);
	/**
	 * Build graph nodes for #AnimData block and any animated images used.
	 * \param id: ID-Block which hosts the #AnimData
	 */
	virtual void build_animdata(ID *id);
	virtual void build_parameters(ID *id);

	virtual void build_action(Action *action);
	virtual void build_armature(Armature *armature);
	virtual void build_collection(LayerCollection *from_layer_collection, Collection *collection);
	virtual void build_object(int base_index, Object *ob, eDepsNode_LinkedState_Type linked_state, bool is_visible);
	virtual void build_view_layer(Scene *scene, ViewLayer *view_layer, eDepsNode_LinkedState_Type linked_state);
	virtual void build_scene_parameters(Scene *scene);

	virtual void build_armature_bones(ListBase *bones);
	virtual void build_object_data(Object *ob);
	virtual void build_object_data_armature(Object *ob);
	virtual void build_object_data_geometry(Object *ob);
	virtual void build_object_data_geometry_datablock(ID *obdata);
	virtual void build_object_flags(int base_index, Object *ob, eDepsNode_LinkedState_Type linked_state);
	virtual void build_object_from_layer(int base_index, Object *ob, eDepsNode_LinkedState_Type linked_state);
	virtual void build_object_transform(Object *ob);
	virtual void build_layer_collections(ListBase *lb);

protected:
	/* Allows to identify an operation which was tagged for update at the time
	 * relations are being updated. We can not reuse operation node pointer
	 * since it will change during dependency graph construction. */
	struct SavedEntryTag {
		ID *id_orig;
		NodeType component_type;
		OperationCode opcode;
		std::string name;
		int name_tag;
	};
	Vector<SavedEntryTag> saved_entry_tags_;

	struct BuilderWalkUserData {
		DepsgraphNodeBuilder *builder;
	};
	static void modifier_walk(void *user_data, struct Object *object, struct ID **idpoin, int cb_flag);

	void tag_previously_tagged_nodes();
	/**
	 * Check for IDs that need to be flushed (COW-updated)
	 * because the depsgraph itself created or removed some of their evaluated dependencies.
	 */
	void update_invalid_cow_pointers();

	/* State which demotes currently built entities. */
	Scene *scene_;
	ViewLayer *view_layer_;
	int view_layer_index_;
	/* NOTE: Collection are possibly built recursively, so be careful when
	 * setting the current state. */
	Collection *collection_;
	/* Accumulated flag over the hierarchy of currently building collections.
	 * Denotes whether all the hierarchy from parent of `collection_` to the
	 * very root is visible (aka not restricted.). */
	bool is_parent_collection_visible_;

	/* Indexed by original ID.session_uuid, values are IDInfo. */
	Map<uint, IDInfo *> id_info_hash_;

	/* Set of IDs which were already build. Makes it easier to keep track of
	 * what was already built and what was not. */
	BuilderMap built_map_;
};

}  // namespace rose::depsgraph

#endif	// !DEG_BUILDER_NODES_HH
