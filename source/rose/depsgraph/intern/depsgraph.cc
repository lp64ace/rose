#include "DEG_depsgraph.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_scene.h"

#include "LIB_thread.h"

#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_factory.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"
#include "intern/node/deg_node_time.hh"

#include "intern/depsgraph_registry.hh"

#include "depsgraph.hh"

namespace rose::depsgraph {

Depsgraph::Depsgraph(Main *main, Scene *scene, ViewLayer *view_layer) : time_source(nullptr), need_update(true), need_visibility_update(true), need_visibility_time_update(false), main(main), scene(scene), view_layer(view_layer), ctime(0.0), scene_cow(nullptr), is_active(false), is_evaluating(false) {
	LIB_spin_init(&lock);
	memset(id_type_updated, 0, sizeof(id_type_updated));
	memset(id_type_exist, 0, sizeof(id_type_exist));

	this->ctime = KER_scene_frame_get(scene);

	add_time_source();
}

Depsgraph::~Depsgraph() {
	clear_id_nodes();
	delete time_source;
	LIB_spin_end(&lock);
}

/* -------------------------------------------------------------------- */
/** \name Node Management
 * \{ */

TimeSourceNode *Depsgraph::add_time_source() {
	if (time_source == nullptr) {
		DepsNodeFactory *factory = type_get_factory(NodeType::TIMESOURCE);
		time_source = (TimeSourceNode *)factory->create_node(nullptr, "", "Time Source");
	}
	return time_source;
}

TimeSourceNode *Depsgraph::find_time_source() const {
	return time_source;
}

void Depsgraph::tag_time_source() {
	time_source->tag_update(this, DEG_UPDATE_SOURCE_TIME);
}

IDNode *Depsgraph::find_id_node(const ID *id) const {
	return id_hash.lookup_default(id, nullptr);
}

IDNode *Depsgraph::add_id_node(ID *id, ID *id_cow_hint) {
	ROSE_assert((id->tag & ID_TAG_COPIED_ON_WRITE) == 0);
	IDNode *id_node = find_id_node(id);
	if (!id_node) {
		DepsNodeFactory *factory = type_get_factory(NodeType::ID_REF);
		id_node = (IDNode *)factory->create_node(id, "", id->name);
		id_node->init_copy_on_write(id_cow_hint);
		/* Register node in ID hash.
		 *
		 * NOTE: We address ID nodes by the original ID pointer they are
		 * referencing to. */
		id_hash.add_new(id, id_node);
		id_nodes.append(id_node);

		id_type_exist[KER_idtype_idcode_to_index(GS(id->name))] = 1;
	}
	return id_node;
}

template<typename FilterFunc> static void clear_id_nodes_conditional(Depsgraph::IDDepsNodes *id_nodes, const FilterFunc &filter) {
	for (IDNode *id_node : *id_nodes) {
		if (id_node->id_cow == nullptr) {
			/**
			 * This means builder "stole" ownership of the copy-on-written
			 * datablock for her own dirty needs.
			 */
			continue;
		}
		if (id_node->id_cow == id_node->id_orig) {
			/**
			 * Copy-on-write version is not needed for this ID type.
			 *
			 * NOTE: Is important to not de-reference the original datablock here because it might be
			 * freed already (happens during main database free when some IDs are freed prior to a
			 * scene).
			 */
			continue;
		}
		if (!deg_copy_on_write_is_expanded(id_node->id_cow)) {
			continue;
		}
		const ID_Type id_type = GS(id_node->id_cow->name);
		if (filter(id_type)) {
			id_node->destroy();
		}
	}
}

void Depsgraph::clear_id_nodes() {
	/* Free memory used by ID nodes. */

	/* Stupid workaround to ensure we free IDs in a proper order. */
	clear_id_nodes_conditional(&id_nodes, [](ID_Type id_type) { return true; });

	for (IDNode *id_node : id_nodes) {
		delete id_node;
	}
	/* Clear containers. */
	id_hash.clear();
	id_nodes.clear();
}

Relation *Depsgraph::add_new_relation(Node *from, Node *to, const char *description, int flags) {
	Relation *rel = nullptr;
	if (flags & RELATION_CHECK_BEFORE_ADD) {
		rel = check_nodes_connected(from, to, description);
	}
	if (rel != nullptr) {
		rel->flag |= static_cast<RelationFlag>(flags);
		return rel;
	}

#ifndef NDEBUG
	if (from->type == NodeType::OPERATION && to->type == NodeType::OPERATION) {
		OperationNode *operation_from = static_cast<OperationNode *>(from);
		OperationNode *operation_to = static_cast<OperationNode *>(to);
		ROSE_assert(operation_to->owner->type != NodeType::COPY_ON_WRITE || operation_from->owner->type == NodeType::COPY_ON_WRITE);
	}
#endif

	/* Create new relation, and add it to the graph. */
	rel = new Relation(from, to, description);
	rel->flag |= static_cast<RelationFlag>(flags);
	return rel;
}

Relation *Depsgraph::check_nodes_connected(const Node *from, const Node *to, const char *description) {
	for (Relation *rel : from->outlinks) {
		ROSE_assert(rel->from == from);
		if (rel->to != to) {
			continue;
		}
		if (description != nullptr && !STREQ(rel->name, description)) {
			continue;
		}
		return rel;
	}
	return nullptr;
}

void Depsgraph::add_entry_tag(OperationNode *node) {
	/* Sanity check. */
	if (node == nullptr) {
		return;
	}
	/**
	 * Add to graph-level set of directly modified nodes to start searching from.
	 * NOTE: this is necessary since we have several thousand nodes to play with.
	 */
	entry_tags.add(node);
}

void Depsgraph::clear_all_nodes() {
	clear_id_nodes();
	delete time_source;
	time_source = nullptr;
}

ID *Depsgraph::get_cow_id(const ID *id_orig) const {
	IDNode *id_node = find_id_node(id_orig);
	if (id_node == nullptr) {
		/* This function is used from places where we expect ID to be either
		 * already a copy-on-write version or have a corresponding copy-on-write
		 * version.
		 *
		 * We try to enforce that in debug builds, for release we play a bit
		 * safer game here. */
		if ((id_orig->tag & ID_TAG_COPIED_ON_WRITE) == 0) {
			/* TODO(sergey): This is nice sanity check to have, but it fails
			 * in following situations:
			 *
			 * - Material has link to texture, which is not needed by new
			 *   shading system and hence can be ignored at construction.
			 * - Object or mesh has material at a slot which is not used (for
			 *   example, object has material slot by materials are set to
			 *   object data). */
			// BLI_assert_msg(0, "Request for non-existing copy-on-write ID");
		}
		return (ID *)id_orig;
	}
	return id_node->id_cow;
}

/** \} */

}  // namespace rose::depsgraph

namespace deg = rose::depsgraph;

/* -------------------------------------------------------------------- */
/** \name Creation & Deletion
 * \{ */

Depsgraph *DEG_graph_new(Main *main, Scene *scene, ViewLayer *view_layer) {
	deg::Depsgraph *deg_depsgraph = new deg::Depsgraph(main, scene, view_layer);
	deg::register_graph(deg_depsgraph);
	return reinterpret_cast<Depsgraph *>(deg_depsgraph);
}

void DEG_graph_move(Depsgraph *depsgraph, Main *main, Scene *scene, ViewLayer *view_layer) {
	deg::Depsgraph *deg_graph = reinterpret_cast<deg::Depsgraph *>(depsgraph);

	const bool do_update_register = deg_graph->main != main;
	if (do_update_register && deg_graph->main != nullptr) {
		deg::unregister_graph(deg_graph);
	}

	deg_graph->main = main;
	deg_graph->scene = scene;
	deg_graph->view_layer = view_layer;

	if (do_update_register) {
		deg::register_graph(deg_graph);
	}
}

void DEG_graph_free(Depsgraph *depsgraph) {
	if (depsgraph == nullptr) {
		return;
	}
	using deg::Depsgraph;
	deg::Depsgraph *deg_depsgraph = reinterpret_cast<deg::Depsgraph *>(depsgraph);
	deg::unregister_graph(deg_depsgraph);
	delete deg_depsgraph;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Evaluation
 * \{ */

bool DEG_is_active(const Depsgraph *depsgraph) {
	if (depsgraph == nullptr) {
		/* Happens for such cases as work object in what_does_obaction(),
		 * and sine render pipeline parts. Shouldn't really be accepting
		 * nullptr depsgraph, but is quite hard to get proper one in those
		 * cases. */
		return false;
	}
	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(depsgraph);
	return deg_graph->is_active;
}

void DEG_make_active(Depsgraph *depsgraph) {
	deg::Depsgraph *deg_graph = reinterpret_cast<deg::Depsgraph *>(depsgraph);
	deg_graph->is_active = true;
}

void DEG_make_inactive(Depsgraph *depsgraph) {
	deg::Depsgraph *deg_graph = reinterpret_cast<deg::Depsgraph *>(depsgraph);
	deg_graph->is_active = false;
}

/** \} */
