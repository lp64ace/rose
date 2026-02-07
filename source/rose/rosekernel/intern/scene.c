#include "MEM_guardedalloc.h"

#include "LIB_ghash.h"
#include "LIB_utildefines.h"

#include "KER_collection.h"
#include "KER_idtype.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_scene.h"

#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph_query.h"

/* -------------------------------------------------------------------- */
/** \name Scene Creation
 * \{ */

Scene *KER_scene_new(Main *main, const char *name) {
	return KER_id_new(main, ID_SCE, name);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scene Render Data
 * \{ */

void KER_scene_time_step(struct Scene *scene, float dt) {
	RenderData *r = &scene->r;

	r->subframe += dt * r->fps;

	int f = (int)r->subframe;

	if (f > 0) {
		r->subframe = r->subframe - (float)f;
		r->cframe += f;

		/**
		 * This should only happen once and is faster than running 
		 * a module here!
		 */
		const int l = r->eframe - r->sframe;
		while (r->cframe >= r->eframe) {
			r->cframe -= l;
		} 
	}
}

float KER_scene_frame_get(const Scene *scene) {
	const RenderData *r = &scene->r;

	return (float)r->cframe + (float)r->subframe;
}

void KER_scene_frame_set(Scene *scene, float ctime) {
	RenderData *r = &scene->r;

	r->cframe = (int)ctime;
	r->subframe = ctime - (float)r->cframe;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scene View Layer Management
 * \{ */

bool KER_scene_has_view_layer(const Scene *scene, const ViewLayer *layer) {
	return LIB_haslink(&scene->view_layers, layer);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Depsgraph Management
 * \{ */

typedef struct DepsgraphKey {
	const ViewLayer *view_layer;
} DepsgraphKey;

ROSE_INLINE unsigned int depsgraph_key_hash(const void *key_v) {
	const DepsgraphKey *key = (const DepsgraphKey *)(key_v);
	unsigned int hash = LIB_ghashutil_ptrhash(key->view_layer);
	return hash;
}

ROSE_INLINE bool depsgraph_key_compare(const void *key_a_v, const void *key_b_v) {
	const DepsgraphKey *key_a = (const DepsgraphKey *)(key_a_v);
	const DepsgraphKey *key_b = (const DepsgraphKey *)(key_b_v);
	return !(key_a->view_layer == key_b->view_layer);
}

ROSE_INLINE void depsgraph_key_free(void *key_v) {
	DepsgraphKey *key = (DepsgraphKey *)(key_v);
	MEM_freeN(key);
}

ROSE_INLINE void depsgraph_key_value_free(void *value) {
	Depsgraph *depsgraph = (Depsgraph *)(value);
	DEG_graph_free(depsgraph);
}

void KER_scene_allocate_depsgraph_hash(Scene *scene) {
	scene->depsgraph_hash = LIB_ghash_new(depsgraph_key_hash, depsgraph_key_compare, "Scene Depsgraph Hash");
}

void KER_scene_ensure_depsgraph_hash(Scene *scene) {
	if (scene->depsgraph_hash == NULL) {
		KER_scene_allocate_depsgraph_hash(scene);
	}
}

void KER_scene_free_depsgraph_hash(Scene *scene) {
	if (scene->depsgraph_hash == NULL) {
		return;
	}
	LIB_ghash_free(scene->depsgraph_hash, depsgraph_key_free, depsgraph_key_value_free);
	scene->depsgraph_hash = NULL;
}

void KER_scene_free_view_layer_depsgraph(Scene *scene, ViewLayer *view_layer) {
	if (scene->depsgraph_hash != NULL) {
		DepsgraphKey key = {view_layer};
		LIB_ghash_remove(scene->depsgraph_hash, &key, depsgraph_key_free, depsgraph_key_value_free);
	}
}

ROSE_INLINE Depsgraph **scene_get_depsgraph_p(Scene *scene, ViewLayer *view_layer, const bool allocate_ghash_entry) {
	ROSE_assert(scene != NULL);
	ROSE_assert(view_layer != NULL);
	ROSE_assert(KER_scene_has_view_layer(scene, view_layer));

	if (allocate_ghash_entry) {
		KER_scene_ensure_depsgraph_hash(scene);
	}
	if (scene->depsgraph_hash == NULL) {
		return NULL;
	}

	DepsgraphKey key;
	key.view_layer = view_layer;

	Depsgraph **depsgraph_ptr;
	if (!allocate_ghash_entry) {
		depsgraph_ptr = (Depsgraph **)LIB_ghash_lookup_p(scene->depsgraph_hash, &key);
		return depsgraph_ptr;
	}

	DepsgraphKey **key_ptr;
	if (LIB_ghash_ensure_p_ex(scene->depsgraph_hash, &key, (void ***)&key_ptr, (void ***)&depsgraph_ptr)) {
		return depsgraph_ptr;
	}

	/* Depsgraph was not found in the ghash, but the key still needs allocating. */
	*key_ptr = MEM_callocN(sizeof(DepsgraphKey), __func__);
	**key_ptr = key;

	*depsgraph_ptr = NULL;
	return depsgraph_ptr;
}

ROSE_INLINE Depsgraph **scene_ensure_depsgraph_p(Main *main, Scene *scene, ViewLayer *view_layer) {
	ROSE_assert(main != NULL);

	Depsgraph **depsgraph_ptr = scene_get_depsgraph_p(scene, view_layer, true);
	if (depsgraph_ptr == NULL) {
		/* The scene has no depsgraph hash. */
		return NULL;
	}
	if (*depsgraph_ptr != NULL) {
		/* The depsgraph was found, no need to allocate. */
		return depsgraph_ptr;
	}

	/**
	 * Allocate a new depsgraph. scene_get_depsgraph_p() already ensured that the pointer is stored
	 * in the scene's depsgraph hash.
	 */
	*depsgraph_ptr = DEG_graph_new(main, scene, view_layer);

	return depsgraph_ptr;
}

Depsgraph *KER_scene_get_depsgraph(const Scene *scene, const ViewLayer *view_layer) {
	ROSE_assert(KER_scene_has_view_layer(scene, view_layer));

	if (scene->depsgraph_hash == NULL) {
		return NULL;
	}

	DepsgraphKey key;
	key.view_layer = view_layer;
	return (Depsgraph *)(LIB_ghash_lookup(scene->depsgraph_hash, &key));
}

Depsgraph *KER_scene_ensure_depsgraph(Main *main, Scene *scene, ViewLayer *view_layer) {
	Depsgraph **depsgraph_ptr = scene_ensure_depsgraph_p(main, scene, view_layer);
	return (depsgraph_ptr != NULL) ? *depsgraph_ptr : NULL;
}

ROSE_INLINE void scene_graph_update_tagged(Depsgraph *depsgraph, Main *main, bool only_if_tagged) {
	if (only_if_tagged && DEG_is_fully_evaluated(depsgraph)) {
		return;
	}

	Scene *scene = DEG_get_input_scene(depsgraph);
	ViewLayer *view_layer = DEG_get_input_view_layer(depsgraph);
	bool used_multiple_passes = false;

	/* (Re-)build dependency graph if needed. */
	DEG_graph_relations_update(depsgraph);
	DEG_evaluate_on_refresh(depsgraph);

	// DEG_editors_update(depsgraph, false);
	DEG_ids_clear_recalc(depsgraph, false);
}

void KER_scene_graph_update_tagged(Depsgraph *depsgraph, Main *main) {
	scene_graph_update_tagged(depsgraph, main, false);
}

void KER_scene_graph_evaluated_ensure(Depsgraph *depsgraph, Main *main) {
	scene_graph_update_tagged(depsgraph, main, true);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Scene Data-block Definition
 * \{ */

ROSE_STATIC void scene_init_data(ID *id) {
	Scene *scene = (Scene *)id;

	/**
	 * Running smoothly for almost 2 years on 30 frames per second.
	 * Should honestly be more than enough.
	 */
	scene->r.sframe = 0;
	scene->r.eframe = INT_MAX;
	scene->r.fps = 30;

	/* Master Collection */
	scene->master_collection = KER_collection_master_add();

	KER_view_layer_add(scene, "ViewLayer", NULL, VIEWLAYER_ADD_NEW);
}

void KER_scene_copy_data(Main *main, Scene *scene_dst, const Scene *scene_src, const int flag) {
	/* We never handle usercount here for own data. */
	const int flag_subdata = flag | LIB_ID_CREATE_NO_USER_REFCOUNT;
	/* We always need allocation of our private ID data. */
	const int flag_private_id_data = flag & ~LIB_ID_CREATE_NO_ALLOCATE;

	scene_dst->depsgraph_hash = NULL;
	/* Master Collection */
	if (scene_src->master_collection) {
		KER_id_copy_ex(main, (ID *)scene_src->master_collection, (ID **)&scene_dst->master_collection, flag_private_id_data);
	}

	/* View Layers */
	LIB_duplicatelist(&scene_dst->view_layers, &scene_src->view_layers);
	for (ViewLayer *view_layer_src = (ViewLayer *)(scene_src->view_layers.first), *view_layer_dst = (ViewLayer *)(scene_dst->view_layers.first); view_layer_src; view_layer_src = view_layer_src->next, view_layer_dst = view_layer_dst->next) {
		KER_view_layer_copy_data(scene_dst, scene_src, view_layer_dst, view_layer_src, flag_subdata);
	}
}

ROSE_STATIC void scene_copy_data(Main *main, ID *id_dst, const ID *id_src, const int flag) {
	KER_scene_copy_data(main, (Scene *)id_dst, (const Scene *)id_src, flag);
}

ROSE_STATIC void scene_free_data(ID *id) {
	Scene *scene = (Scene *)id;

	LISTBASE_FOREACH_MUTABLE(ViewLayer *, view_layer, &scene->view_layers) {
		LIB_remlink(&scene->view_layers, view_layer);
		KER_view_layer_free_ex(view_layer, false);
	}

	KER_scene_free_depsgraph_hash(scene);

	/* Master Collection */
	/* TODO: what to do with do_id_user? it's also true when just
	 * closing the file which seems wrong? should decrement users
	 * for objects directly in the master collection? then other
	 * collections in the scene need to do it too? */
	if (scene->master_collection) {
		KER_collection_free_data(scene->master_collection);
		MEM_freeN(scene->master_collection);
		scene->master_collection = NULL;
	}
}

ROSE_STATIC void scene_foreach_layer_collection(struct LibraryForeachIDData *data, ListBase *lb, const bool is_master) {
	const int data_flags = KER_lib_query_foreachid_process_flags_get(data);

	LISTBASE_FOREACH(LayerCollection *, lc, lb) {
		KER_LIB_FOREACHID_PROCESS_IDSUPER(data, lc->collection, IDWALK_CB_NOP);
		scene_foreach_layer_collection(data, &lc->layer_collections, false);
	}
}

ROSE_STATIC void scene_foreach_id(ID *id, struct LibraryForeachIDData *data) {
	Scene *scene = (Scene *)(id);
	const int flag = KER_lib_query_foreachid_process_flags_get(data);

	KER_LIB_FOREACHID_PROCESS_IDSUPER(data, scene->camera, IDWALK_CB_NOP);
	/* This pointer can be nullptr during old files reading, better be safe than sorry. */
	if (scene->master_collection != NULL) {
		KER_LIB_FOREACHID_PROCESS_ID(data, scene->master_collection, IDWALK_CB_NOP);
	}

	LISTBASE_FOREACH(ViewLayer *, view_layer, &scene->view_layers) {
		LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
			KER_LIB_FOREACHID_PROCESS_IDSUPER(data, base->object, IDWALK_CB_NOP);
		}

		KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data, scene_foreach_layer_collection(data, &view_layer->layer_collections, true));
	}
}

 IDTypeInfo IDType_ID_SCE = {
	.idcode = ID_SCE,

	.filter = FILTER_ID_SCE,
	.depends = 0,
	.index = INDEX_ID_SCE,
	.size = sizeof(Scene),

	.name = "Scene",
	.name_plural = "Scenes",

	.flag = 0,

	.init_data = scene_init_data,
	.copy_data = scene_copy_data,
	.free_data = scene_free_data,

	.foreach_id = scene_foreach_id,

	.write = NULL,
	.read_data = NULL,
};

/** \} */

