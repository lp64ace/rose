#ifndef DEPSGRAPH_HH
#define DEPSGRAPH_HH

#include "MEM_guardedalloc.h"

#include "LIB_map.hh"
#include "LIB_set.hh"
#include "LIB_vector.hh"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_query.h"

namespace rose::depsgraph {

struct Node;
struct IDNode;
struct Relation;
struct OperationNode;
struct TimeSourceNode;

struct Depsgraph {
	typedef rose::Vector<OperationNode *> OperationNodes;
	typedef rose::Vector<IDNode *> IDDepsNodes;

	Depsgraph(Main *main, Scene *scene, ViewLayer *view_layer);
	~Depsgraph();

	TimeSourceNode *add_time_source();
	TimeSourceNode *find_time_source() const;
	void tag_time_source();

	IDNode *find_id_node(const ID *id) const;
	IDNode *add_id_node(ID *id, ID *id_cow_hint = nullptr);
	void clear_id_nodes();

	/** Add new relationship between two nodes. */
	Relation *add_new_relation(Node *from, Node *to, const char *description, int flags = 0);

	/* Check whether two nodes are connected by relation with given
	 * description. Description might be nullptr to check ANY relation between
	 * given nodes. */
	Relation *check_nodes_connected(const Node *from, const Node *to, const char *description);

	/* Tag a specific node as needing updates. */
	void add_entry_tag(OperationNode *node);

	/* Clear storage used by all nodes. */
	void clear_all_nodes();

	/* Copy-on-Write Functionality ........ */

	/* For given original ID get ID which is created by CoW system. */
	ID *get_cow_id(const ID *id_orig) const;

	/* <ID : IDNode> mapping from ID blocks to nodes representing these
	 * blocks, used for quick lookups. */
	Map<const ID *, IDNode *> id_hash;

	/* Ordered list of ID nodes, order matches ID allocation order.
	 * Used for faster iteration, especially for areas which are critical to
	 * keep exact order of iteration. */
	IDDepsNodes id_nodes;

	/* Top-level time source node. */
	TimeSourceNode *time_source;

	/* Indicates whether relations needs to be updated. */
	bool need_update;

	/* Indicated whether IDs in this graph are to be tagged as if they first appear visible, with
	 * an optional tag for their animation (time) update. */
	bool need_visibility_update;
	bool need_visibility_time_update;

	/* Indicates which ID types were updated. */
	char id_type_updated[INDEX_ID_MAX];
	/* Indicates type of IDs present in the depsgraph. */
	char id_type_exist[INDEX_ID_MAX];

	/* Quick-Access Temp Data ............. */

	/* Nodes which have been tagged as "directly modified". */
	Set<OperationNode *> entry_tags;

	/* Spin lock for threading-critical operations.
	 * Mainly used by graph evaluation. */
	SpinLock lock;

	/* Convenience Data ................... */

	/* XXX: should be collected after building (if actually needed?) */
	/* All operation nodes, sorted in order of single-thread traversal order. */
	OperationNodes operations;

	/* Main, scene, layer, mode this dependency graph is built for. */
	Main *main;
	Scene *scene;
	ViewLayer *view_layer;

	/**
	 * Time at which dependency graph is being or was last evaluated.
	 * frame is the value before, and ctime the value after time remapping.
	 */
	float ctime;

	/**
	 * Evaluated version of datablocks we access a lot.
	 * Stored here to save us form doing hash lookup.
	 */
	Scene *scene_cow;

	/**
	 * Active dependency graph is a dependency graph which is used by the
	 * currently active window. When dependency graph is active, it is allowed
	 * for evaluation functions to write animation f-curve result, drivers
	 * result and other selective things (object matrix?) to original object.
	 *
	 * This way we simplify operators, which don't need to worry about where
	 * to read stuff from.
	 */
	bool is_active;

	bool is_evaluating;
};

}  // namespace rose::depsgraph

#endif	// DEPSGRAPH_HH
