#ifndef DEG_BUILDER_RELATIONS_HH
#define DEG_BUILDER_RELATIONS_HH

#include <cstdio>
#include <cstring>

#include "intern/depsgraph_types.hh"

#include "DNA_ID.h"

#include "RNA_access.h"
#include "RNA_types.h"

#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "intern/builder/deg_builder.hh"
#include "intern/builder/deg_builder_map.hh"
#include "intern/builder/deg_builder_rna.hh"
#include "intern/depsgraph.hh"
#include "intern/node/deg_node.hh"
#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"

struct Armature;
struct LayerCollection;
struct Collection;

namespace rose::depsgraph {

struct ComponentNode;
struct DepsNodeHandle;
struct Depsgraph;
class DepsgraphBuilderCache;
struct IDNode;
struct Node;
struct OperationNode;
struct Relation;
struct TimeSourceNode;

struct TimeSourceKey {
	TimeSourceKey();
	TimeSourceKey(ID *id);

	std::string identifier() const;

	ID *id;
};

struct ComponentKey {
	ComponentKey();
	ComponentKey(ID *id, NodeType type, const char *name = "");

	std::string identifier() const;

	ID *id;
	NodeType type;
	const char *name;
};

struct OperationKey {
	OperationKey();
	OperationKey(ID *id, NodeType component_type, const char *name, int name_tag = -1);
	OperationKey(ID *id, NodeType component_type, const char *component_name, const char *name, int name_tag);

	OperationKey(ID *id, NodeType component_type, OperationCode opcode);
	OperationKey(ID *id, NodeType component_type, const char *component_name, OperationCode opcode);

	OperationKey(ID *id, NodeType component_type, OperationCode opcode, const char *name, int name_tag = -1);
	OperationKey(ID *id, NodeType component_type, const char *component_name, OperationCode opcode, const char *name, int name_tag = -1);

	std::string identifier() const;

	ID *id;
	NodeType component_type;
	const char *component_name;
	OperationCode opcode;
	const char *name;
	int name_tag;
};

struct RNAPathKey {
	RNAPathKey(ID *id, const char *path, RNAPointerSource source);
	RNAPathKey(ID *id, const PointerRNA &ptr, PropertyRNA *prop, RNAPointerSource source);

	std::string identifier() const;

	ID *id;
	PointerRNA ptr;
	PropertyRNA *prop;
	RNAPointerSource source;
};

class DepsgraphRelationBuilder : public DepsgraphBuilder {
public:
	DepsgraphRelationBuilder(Main *bmain, Depsgraph *graph, DepsgraphBuilderCache *cache);

	template<typename KeyFrom, typename KeyTo> Relation *add_relation(const KeyFrom &key_from, const KeyTo &key_to, const char *description, int flags = 0);
	template<typename KeyTo> Relation *add_relation(const TimeSourceKey &key_from, const KeyTo &key_to, const char *description, int flags = 0);
	template<typename KeyType> Relation *add_node_handle_relation(const KeyType &key_from, const DepsNodeHandle *handle, const char *description, int flags = 0);
	template<typename KeyTo> Relation *add_depends_on_transform_relation(ID *id, const KeyTo &key_to, const char *description, int flags = 0);

	void begin_build();

	virtual void build_copy_on_write_relations();
	virtual void build_copy_on_write_relations(IDNode *id_node);

	virtual void build_id(ID *id);
	virtual void build_generic_id(ID *id);
	virtual void build_idproperties(IDProperty *idproperty);
	virtual void build_animdata_curves_targets(ID *id, ComponentKey &adt_key, OperationNode *operation_from, FCurve **curves, int totcurve);
	virtual void build_animdata_curves(ID *id);
	virtual void build_animdata(ID *id);
	virtual void build_parameters(ID *id);

	virtual void build_action(Action *action);
	virtual void build_armature(Armature *armature);
	virtual void build_armature_bones(ListBase *bones);
	virtual void build_collection(LayerCollection *from_layer_collection, Object *object, Collection *collection);
	virtual void build_object(Object *object);
	virtual void build_object_dimensions(Object *object);
	virtual void build_object_from_view_layer_base(Object *object);
	virtual void build_object_layer_component_relations(Object *object);
	virtual void build_object_data(Object *object);
	virtual void build_object_parent(Object *object);
	virtual void build_object_data_armature(Object *object);
	virtual void build_object_data_geometry(Object *object);
	virtual void build_object_data_geometry_datablock(ID *obdata);
	virtual void build_scene_parameters(Scene *scene);
	virtual void build_view_layer(Scene *scene, ViewLayer *view_layer, eDepsNode_LinkedState_Type linked_state);
	virtual void build_layer_collections(ListBase *lb);

	template<typename KeyType> OperationNode *find_operation_node(const KeyType &key);

protected:
	TimeSourceNode *get_node(const TimeSourceKey &key) const;
	ComponentNode *get_node(const ComponentKey &key) const;
	OperationNode *get_node(const OperationKey &key) const;
	Node *get_node(const RNAPathKey &key);

	OperationNode *find_node(const OperationKey &key) const;
	bool has_node(const OperationKey &key) const;

	Relation *add_time_relation(TimeSourceNode *timesrc, Node *node_to, const char *description, int flags = 0);
	Relation *add_operation_relation(OperationNode *node_from, OperationNode *node_to, const char *description, int flags = 0);
	/* Add relation which ensures visibility of `id_from` when `id_to` is visible.
	 * For the more detailed explanation see comment for `NodeType::VISIBILITY`. */
	void add_visibility_relation(ID *id_from, ID *id_to);

private:
	struct BuilderWalkUserData {
		DepsgraphRelationBuilder *builder;
	};

	static void modifier_walk(void *user_data, struct Object *object, struct ID **idpoin, int cb_flag);

	/* State which demotes currently built entities. */
	Scene *scene_;

	BuilderMap built_map_;
	RNANodeQuery rna_node_query_;
};

struct DepsNodeHandle {
	DepsNodeHandle(DepsgraphRelationBuilder *builder, OperationNode *node, const char *default_name = "") : builder(builder), node(node), default_name(default_name) {
		ROSE_assert(node != nullptr);
	}

	DepsgraphRelationBuilder *builder;
	OperationNode *node;
	const char *default_name;
};

}  // namespace rose::depsgraph

#include "deg_builder_relations_impl.hh"

#endif	// !DEG_BUILDER_RELATIONS_HH
