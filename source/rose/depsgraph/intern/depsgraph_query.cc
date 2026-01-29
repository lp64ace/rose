#include "DEG_depsgraph_query.h"

#include "KER_idtype.h"
#include "KER_layer.h"
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

namespace deg = rose::depsgraph;

/* -------------------------------------------------------------------- */
/** \name DEG input data
 * \{ */

Scene *DEG_get_input_scene(const Depsgraph *graph) {
	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(graph);
	return deg_graph->scene;
}

ViewLayer *DEG_get_input_view_layer(const Depsgraph *graph) {
	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(graph);
	return deg_graph->view_layer;
}

Main *DEG_get_main(const Depsgraph *graph) {
	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(graph);
	return deg_graph->main;
}

float DEG_get_ctime(const Depsgraph *graph) {
	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(graph);
	return deg_graph->ctime;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name DEG evaluated data
 * \{ */

bool DEG_id_type_updated(const Depsgraph *graph, short id_type) {
	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(graph);
	return deg_graph->id_type_updated[KER_idtype_idcode_to_index(id_type)] != 0;
}

bool DEG_id_type_any_updated(const struct Depsgraph *graph) {
	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(graph);

	/* Loop over all ID types. */
	for (char id_type_index : deg_graph->id_type_updated) {
		if (id_type_index) {
			return true;
		}
	}

	return false;
}

bool DEG_is_fully_evaluated(const Depsgraph *depsgraph) {
	const deg::Depsgraph *deg_graph = (const deg::Depsgraph *)depsgraph;
	/* Check whether relations are up to date. */
	if (deg_graph->need_update) {
		return false;
	}
	/* Check whether IDs are up to date. */
	if (!deg_graph->entry_tags.is_empty()) {
		return false;
	}
	return true;
}

ID *DEG_get_original_id(ID *id) {
	if (id == nullptr) {
		return nullptr;
	}
	if (id->orig_id == nullptr) {
		return id;
	}
	ROSE_assert((id->tag & ID_TAG_COPIED_ON_WRITE) != 0);
	return (ID *)id->orig_id;
}

Object *DEG_get_original_object(Object *object) {
	return (Object *)DEG_get_original_id(&object->id);
}

void DEG_get_customdata_mask_for_object(const Depsgraph *graph, Object *object, CustomData_MeshMasks *r_mask) {
	if (graph == nullptr) {
		/* Happens when converting objects to mesh from a python script
		 * after modifying scene graph.
		 *
		 * Currently harmless because it's only called for temporary
		 * objects which are out of the DAG anyway. */
		return;
	}

	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(graph);
	const deg::IDNode *id_node = deg_graph->find_id_node(DEG_get_original_id(&object->id));
	if (id_node == nullptr) {
		/* TODO(sergey): Does it mean we need to check set scene? */
		return;
	}

	r_mask->vmask |= id_node->customdata_masks.vert_mask;
	r_mask->emask |= id_node->customdata_masks.edge_mask;
	r_mask->fmask |= id_node->customdata_masks.face_mask;
	r_mask->lmask |= id_node->customdata_masks.loop_mask;
	r_mask->pmask |= id_node->customdata_masks.poly_mask;
}

Scene *DEG_get_evaluated_scene(const Depsgraph *graph) {
	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(graph);
	Scene *scene_cow = deg_graph->scene_cow;
	/* TODO(sergey): Shall we expand data-block here? Or is it OK to assume
	 * that caller is OK with just a pointer in case scene is not updated yet? */
	ROSE_assert(scene_cow != nullptr && deg::deg_copy_on_write_is_expanded(&scene_cow->id));
	return scene_cow;
}

ViewLayer *DEG_get_evaluated_view_layer(const Depsgraph *graph) {
	const deg::Depsgraph *deg_graph = reinterpret_cast<const deg::Depsgraph *>(graph);
	Scene *scene_cow = DEG_get_evaluated_scene(graph);
	if (scene_cow == nullptr) {
		return nullptr; /* Happens with new, not-yet-built/evaluated graphs. */
	}
	/* Do name-based lookup. */
	/* TODO(sergey): Can this be optimized? */
	ViewLayer *view_layer_orig = deg_graph->view_layer;
	ViewLayer *view_layer_cow = (ViewLayer *)LIB_findstr(&scene_cow->view_layers, view_layer_orig->name, offsetof(ViewLayer, name));
	ROSE_assert(view_layer_cow != nullptr);
	return view_layer_cow;
}

Object *DEG_get_evaluated_object(const Depsgraph *depsgraph, Object *object) {
	return (Object *)DEG_get_evaluated_id(depsgraph, &object->id);
}

ID *DEG_get_evaluated_id(const Depsgraph *depsgraph, ID *id) {
	if (id == nullptr) {
		return nullptr;
	}
	/* TODO(sergey): This is a duplicate of Depsgraph::get_cow_id(),
	 * but here we never do assert, since we don't know nature of the
	 * incoming ID data-block. */
	const deg::Depsgraph *deg_graph = (const deg::Depsgraph *)depsgraph;
	const deg::IDNode *id_node = deg_graph->find_id_node(id);
	if (id_node == nullptr) {
		return id;
	}
	return id_node->id_cow;
}

/** \} */
