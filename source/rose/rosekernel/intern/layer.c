#include "MEM_guardedalloc.h"

#include "KER_collection.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_object.h"
#include "KER_scene.h"

#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_mempool.h"
#include "LIB_string.h"
#include "LIB_string_utils.h"
#include "LIB_thread.h"

#include "DEG_depsgraph.h"

#include <stdio.h>

static const short g_base_collection_flags = (BASE_VISIBLE_DEPSGRAPH | BASE_VISIBLE_VIEWLAYER | BASE_SELECTABLE | BASE_ENABLED_VIEWPORT | BASE_ENABLED_RENDER);

/* -------------------------------------------------------------------- */
/** \name Layer Collections and Bases
 * \{ */

ROSE_STATIC LayerCollection *layer_collection_add(ListBase *lb, Collection *collection) {
	LayerCollection *lc = MEM_callocN(sizeof(LayerCollection), "LayerCollection");
	lc->collection = collection;
	lc->local_collections_bits = ~0u;
	LIB_addtail(lb, lc);
	return lc;
}

ROSE_STATIC void layer_collection_free(ViewLayer *view_layer, LayerCollection *lc) {
	if (lc == view_layer->active_collection) {
		view_layer->active_collection = NULL;
	}
	LISTBASE_FOREACH(LayerCollection *, nlc, &lc->layer_collections) {
		layer_collection_free(view_layer, nlc);
	}
	LIB_freelistN(&lc->layer_collections);
}

ROSE_STATIC Base *object_base_new(Object *object) {
	Base *base = MEM_callocN(sizeof(Base), "Object Base");
	base->object = object;
	base->local_view_bits = ~0u;
	if ((object->flag & OBJECT_SELECTED) != 0) {
		base->flag |= BASE_SELECTED;
	}
	return base;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Base
 * \{ */

ROSE_STATIC void view_layer_bases_hash_create(ViewLayer *view_layer, const bool do_base_duplicates_fix) {
	static ThreadMutex hash_lock = ROSE_MUTEX_INITIALIZER;

	if (view_layer->object_bases_hash == NULL) {
		LIB_mutex_lock(&hash_lock);
		if (view_layer->object_bases_hash == NULL) {
			GHash *hash = LIB_ghash_new(LIB_ghashutil_ptrhash, LIB_ghashutil_ptrcmp, "ViewLayerBasesHash");

			LISTBASE_FOREACH_MUTABLE(Base *, base, &view_layer->bases) {
				if (base->object) {
					void **val_pp;
					if (!LIB_ghash_ensure_p(hash, base->object, &val_pp)) {
						*val_pp = base;
					}
					/**
					 * The same object has several bases.
					 *
					 * In normal cases this is a serious bug,
					 * but this is a common situation when remapping an object into another one already present in the same ViewLayer.
					 * While ideally we would process this case separately, for perfomances reasons it makes more sense to tackle it here.
					 */
					else if (do_base_duplicates_fix) {
						if (view_layer->active == base) {
							view_layer->active = NULL;
						}
						LIB_remlink(&view_layer->bases, base);
						MEM_freeN(base);
					}
					else {
						fprintf(stderr, "[Kernel] Object %s has more than one entry in view layer's object bases listbase", base->object->id.name + 2);
					}
				}
			}
			view_layer->object_bases_hash = hash;
		}
		LIB_mutex_unlock(&hash_lock);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name View Layer
 * \{ */

ViewLayer *KER_view_layer_default_view(const Scene *scene) {
	LISTBASE_FOREACH(ViewLayer *, view_layer, &scene->view_layers) {
		if ((view_layer->flag & VIEW_LAYER_RENDER) == 0) {
			return view_layer;
		}
	}
	ROSE_assert(scene->view_layers.first);
	return (ViewLayer *)scene->view_layers.first;
}

ViewLayer *KER_view_layer_default_render(const Scene *scene) {
	LISTBASE_FOREACH(ViewLayer *, view_layer, &scene->view_layers) {
		if ((view_layer->flag & VIEW_LAYER_RENDER) != 0) {
			return view_layer;
		}
	}
	ROSE_assert(scene->view_layers.first);
	return (ViewLayer *)scene->view_layers.first;
}

ViewLayer *KER_view_layer_find(const Scene *scene, const char *layer_name) {
	LISTBASE_FOREACH(ViewLayer *, view_layer, &scene->view_layers) {
		if (STREQ(view_layer->name, layer_name)) {
			return view_layer;
		}
	}
	return NULL;
}

ROSE_STATIC ViewLayer *view_layer_add(const char *name) {
	ViewLayer *view_layer = MEM_callocN(sizeof(ViewLayer), "ViewLayer");
	view_layer->flag = VIEW_LAYER_RENDER;

	LIB_strcpy(view_layer->name, ARRAY_SIZE(view_layer->name), (name) ? name : "ViewLayer");
	return view_layer;
}

ROSE_STATIC void layer_collection_exclude_all(LayerCollection *layer_collection) {
	LayerCollection *sub_collection = layer_collection->layer_collections.first;
	for (; sub_collection != NULL; sub_collection = sub_collection->next) {
		sub_collection->flag |= LAYER_COLLECTION_EXCLUDE;
		layer_collection_exclude_all(sub_collection);
	}
}

ViewLayer *KER_view_layer_add(Scene *scene, const char *name, ViewLayer *view_layer_source, int type) {
	ViewLayer *view_layer_new = NULL;

	if (view_layer_source) {
		name = view_layer_source->name;
	}

	switch (type) {
		case VIEWLAYER_ADD_NEW: {
			view_layer_new = view_layer_add(name);
			LIB_addtail(&scene->view_layers, view_layer_new);
			KER_layer_collection_sync(scene, view_layer_new);
		} break;
		case VIEWLAYER_ADD_COPY: {
			ROSE_assert(view_layer_source); /** Source layer is required when copying! */

			view_layer_new = MEM_callocN(sizeof(ViewLayer), "ViewLayer");
			memcpy(view_layer_new, view_layer_source, sizeof(ViewLayer));
			KER_view_layer_copy_data(scene, scene, view_layer_new, view_layer_source, 0);
			LIB_addtail(&scene->view_layers, view_layer_new);

			LIB_strcpy(view_layer_new->name, ARRAY_SIZE(view_layer_new->name), name);
		} break;
		case VIEWLAYER_ADD_EMPTY: {
			view_layer_new = view_layer_add(name);
			LIB_addtail(&scene->view_layers, view_layer_new);

			KER_layer_collection_sync(scene, view_layer_new);
			layer_collection_exclude_all(view_layer_new->layer_collections.first);
			KER_layer_collection_sync(scene, view_layer_new);
		} break;
	}

	LIB_uniquename(&scene->view_layers, view_layer_new, "ViewLayer", '_', offsetof(ViewLayer, name), ARRAY_SIZE(view_layer_new->name));
	return view_layer_new;
}

void KER_view_layer_free(ViewLayer *view_layer) {
	KER_view_layer_free_ex(view_layer, true);
}

void KER_view_layer_free_ex(ViewLayer *view_layer, bool us) {
	view_layer->active = NULL;

	LIB_freelistN(&view_layer->bases);
	if (view_layer->object_bases_hash) {
		LIB_ghash_free(view_layer->object_bases_hash, NULL, NULL);
	}

	LISTBASE_FOREACH(LayerCollection *, lc, &view_layer->layer_collections) {
		layer_collection_free(view_layer, lc);
	}
	LIB_freelistN(&view_layer->layer_collections);

	LISTBASE_FOREACH(void *, dd, &view_layer->drawdata) {
		fprintf(stderr, "[Kernel] ViewLayer - drawing engine data need to be freed!");
	}
	LIB_freelistN(&view_layer->drawdata);

	MEM_SAFE_FREE(view_layer->object_bases_array);
	MEM_freeN(view_layer);
}

void KER_view_layer_selected_objects_tag(ViewLayer *view_layer, int tag) {
	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		if ((base->flag & BASE_SELECTED) != 0) {
			base->object->flag |= tag;
		}
		else {
			base->object->flag &= ~tag;
		}
	}
}

ROSE_STATIC bool find_scene_collection_in_scene_collections(ListBase *lb, const LayerCollection *lc) {
	LISTBASE_FOREACH(LayerCollection *, iter, lb) {
		if (iter == lc) {
			return true;
		}
		if (find_scene_collection_in_scene_collections(&iter->layer_collections, lc)) {
			return true;
		}
	}
	return false;
}

Base *KER_view_layer_base_find(ViewLayer *view_layer, Object *object) {
	if (!view_layer->object_bases_hash) {
		view_layer_bases_hash_create(view_layer, false);
	}
	return LIB_ghash_lookup(view_layer->object_bases_hash, object);
}

void KER_view_layer_base_deselect_all(ViewLayer *view_layer) {
	for (Base *base = view_layer->bases.first; base; base = base->next) {
		base->flag &= ~BASE_SELECTED;
	}
}
void KER_view_layer_base_select_and_set_active(ViewLayer *view_layer, Base *selbase) {
	view_layer->active = selbase;
	if ((selbase->flag & BASE_SELECTED) != 0) {
		selbase->flag |= BASE_SELECTED;
	}
}

ROSE_STATIC LayerCollection *find_layer_collection_by_scene_collection(LayerCollection *lc, const Collection *collection) {
	if (lc->collection == collection) {
		return lc;
	}
	LISTBASE_FOREACH(LayerCollection *, nlc, &lc->layer_collections) {
		LayerCollection *found = find_layer_collection_by_scene_collection(nlc, collection);
		if (found) {
			return found;
		}
	}
	return NULL;
}

LayerCollection *KER_layer_collection_first_from_scene_collection(const ViewLayer *view_layer, const Collection *collection) {
	LISTBASE_FOREACH(LayerCollection *,layer_collection, &view_layer->layer_collections) {
		LayerCollection *found = find_layer_collection_by_scene_collection(layer_collection, collection);
		if (found != NULL) {
			return found;
		}
	}
	return NULL;
}

bool KER_view_layer_has_collection(const ViewLayer *view_layer, const Collection *collection) {
	return KER_layer_collection_first_from_scene_collection(view_layer, collection) != NULL;
}

bool KER_scene_has_object(Scene *scene, Object *object) {
	LISTBASE_FOREACH(ViewLayer *, view_layer, &scene->view_layers) {
		Base *base = KER_view_layer_base_find(view_layer, object);
		if (base) {
			return true;
		}
	}
	return false;
}

ROSE_STATIC bool layer_collection_hidden(ViewLayer *view_layer, LayerCollection *lc) {
	if (lc->flag & LAYER_COLLECTION_EXCLUDE) {
		return true;
	}
	if (lc->flag & LAYER_COLLECTION_HIDE || lc->collection->flag & COLLECTION_HIDE_VIEWPORT) {
		return true;
	}
	CollectionParent *parent = lc->collection->parents.first;
	if (parent) {
		lc = KER_layer_collection_first_from_scene_collection(view_layer, parent->collection);

		return lc && layer_collection_hidden(view_layer, lc);
	}
	return false;
}

LayerCollection *KER_layer_collection_get_active(ViewLayer *view_layer) {
	return view_layer->active_collection;
}

bool KER_layer_collection_activate(ViewLayer *view_layer, LayerCollection *lc) {
	if (lc->flag & LAYER_COLLECTION_EXCLUDE) {
		return false;
	}
	view_layer->active_collection = lc;
	return true;
}

LayerCollection *KER_layer_collection_activate_parent(ViewLayer *view_layer, LayerCollection *lc) {
	CollectionParent *parent = lc->collection->parents.first;

	if (parent) {
		lc = KER_layer_collection_first_from_scene_collection(view_layer, parent->collection);
	}
	else {
		lc = NULL;
	}

	if (lc && layer_collection_hidden(view_layer, lc)) {
		return KER_layer_collection_activate_parent(view_layer, lc);
	}

	if (!lc) {
		lc = view_layer->layer_collections.first;
	}

	view_layer->active_collection = lc;
	return lc;
}

ROSE_STATIC size_t collection_count(const ListBase *lb) {
	size_t i = 0;
	LISTBASE_FOREACH(const LayerCollection *, lc, lb) {
		i += collection_count(&lc->layer_collections) + 1;
	}
	return i;
}

size_t KER_layer_collection_count(const struct ViewLayer *view_layer) {
	return collection_count(&view_layer->layer_collections);
}

ROSE_STATIC size_t index_from_collection(const ListBase *lb, const LayerCollection *lc, size_t *i) {
	LISTBASE_FOREACH(LayerCollection *, lcol, lb) {
		if (lcol == lc) {
			return *i;
		}
		++(*i);
	}

	LISTBASE_FOREACH(LayerCollection *, lcol, lb) {
		size_t i_nested = index_from_collection(&lcol->layer_collections, lc, i);
		if (i_nested != -1) {
			return i_nested;
		}
	}
	return (size_t)-1;
}

size_t KER_layer_collection_findindex(const ViewLayer *view_layer, const LayerCollection *lc) {
	size_t i = 0;
	return index_from_collection(&view_layer->layer_collections, lc, &i);
}

ROSE_STATIC LayerCollection *collection_from_index(ListBase *lb, const size_t number, size_t *i) {
	LISTBASE_FOREACH(LayerCollection *, lc, lb) {
		if (*i == number) {
			return lc;
		}
		++(*i);
	}

	LISTBASE_FOREACH(LayerCollection *, lc, lb) {
		LayerCollection *lc_nested = collection_from_index(&lc->layer_collections, number, i);
		if (lc_nested) {
			return lc_nested;
		}
	}
	return NULL;
}

LayerCollection *KER_layer_collection_from_index(ViewLayer *view_layer, size_t index) {
	size_t i = 0;
	return collection_from_index(&view_layer->layer_collections, index, &i);
}

/**
 * The layer collection tree mirrors the scene collection tree. Whenever that 
 * changes we need to synchronize them so that there is a corresponding layer 
 * collection for each collection. Note that the scene collection tree can 
 * contain link or override collections, and so this is also called on .rose 
 * file load to ensure any new or removed collections are synced.
 * 
 * The view layer also contains a list of bases for each object that exists 
 * in at least one layer collection. That list is also synchronized here, and 
 * stores state like selection.
 * 
 * This API allows to temporarily forbid resync of LayerCollections.
 * 
 * This can greatly improve perfomances in cases where those functions get 
 * called a lot (e.g. during massive remappings of IDs).
 * 
 * Usage of these should be done very carefully though. In particular, calling 
 * code must ensures it resync LayerCollections before an UI/Event loop 
 * handling can happen.
 * 
 * \warning This is not threadsafe at all, only use from main thread.
 */

static bool no_resync = false;

void KER_layer_collection_resync_forbid(void) {
	no_resync = true;
}
void KER_layer_collection_resync_allow(void) {
	no_resync = false;
}

/* -------------------------------------------------------------------- */
/** \name Collection Isolation & Local View
 * \{ */

ROSE_STATIC void layer_collection_flag_set_recursive(LayerCollection *lc, const int flag) {
	lc->flag |= flag;
	LISTBASE_FOREACH(LayerCollection *, lc_iter, &lc->layer_collections) {
		layer_collection_flag_set_recursive(lc_iter, flag);
	}
}

ROSE_STATIC void layer_collection_flag_unset_recursive(LayerCollection *lc, const int flag) {
	lc->flag &= ~flag;
	LISTBASE_FOREACH(LayerCollection *, lc_iter, &lc->layer_collections) {
		layer_collection_flag_unset_recursive(lc_iter, flag);
	}
}

void KER_layer_collection_isolate_global(Scene *scene, ViewLayer *view_layer, LayerCollection *lc, bool extend) {
	LayerCollection *lc_master = view_layer->layer_collections.first;
	bool hide_it = extend && (lc->runtime_flag & LAYER_COLLECTION_VISIBLE_VIEW_LAYER);

	if (!extend) {
		/* Hide all collections. */
		LISTBASE_FOREACH(LayerCollection *, lc_iter, &lc_master->layer_collections) {
			layer_collection_flag_set_recursive(lc_iter, LAYER_COLLECTION_HIDE);
		}
	}

	/* Make all the direct parents visible. */
	if (hide_it) {
		lc->flag |= LAYER_COLLECTION_HIDE;
	}
	else {
		LayerCollection *lc_parent = lc;
		LISTBASE_FOREACH(LayerCollection *, lc_iter, &lc_master->layer_collections) {
			if (KER_layer_collection_has_layer_collection(lc_iter, lc)) {
				lc_parent = lc_iter;
				break;
			}
		}

		while (lc_parent != lc) {
			lc_parent->flag &= ~LAYER_COLLECTION_HIDE;

			LISTBASE_FOREACH(LayerCollection *, lc_iter, &lc_parent->layer_collections) {
				if (KER_layer_collection_has_layer_collection(lc_iter, lc)) {
					lc_parent = lc_iter;
					break;
				}
			}
		}

		/* Make all the children visible, but respect their disable state. */
		layer_collection_flag_unset_recursive(lc, LAYER_COLLECTION_HIDE);

		KER_layer_collection_activate(view_layer, lc);
	}

	KER_layer_collection_sync(scene, view_layer);
}

ROSE_STATIC void layer_collection_local_visibility_set_recursive(LayerCollection *layer_collection, const int local_collections_uuid) {
	layer_collection->local_collections_bits |= local_collections_uuid;
	LISTBASE_FOREACH(LayerCollection *, child, &layer_collection->layer_collections) {
		layer_collection_local_visibility_set_recursive(child, local_collections_uuid);
	}
}

ROSE_STATIC void layer_collection_local_visibility_unset_recursive(LayerCollection *layer_collection, const int local_collections_uuid) {
	layer_collection->local_collections_bits &= ~local_collections_uuid;
	LISTBASE_FOREACH(LayerCollection *, child, &layer_collection->layer_collections) {
		layer_collection_local_visibility_unset_recursive(child, local_collections_uuid);
	}
}

ROSE_STATIC void layer_collection_local_sync(ViewLayer *view_layer, LayerCollection *layer_collection, const unsigned short local_collections_uuid, bool visible) {
	if ((layer_collection->local_collections_bits & local_collections_uuid) == 0) {
		visible = false;
	}

	if (visible) {
		LISTBASE_FOREACH(CollectionObject *, cob, &layer_collection->collection->objects) {
			if (cob->object == NULL) {
				continue;
			}

			Base *base = KER_view_layer_base_find(view_layer, cob->object);
			base->local_collections_bits |= local_collections_uuid;
		}
	}

	LISTBASE_FOREACH(LayerCollection *, child, &layer_collection->layer_collections) {
		if ((child->flag & LAYER_COLLECTION_EXCLUDE) == 0) {
			layer_collection_local_sync(view_layer, child, local_collections_uuid, visible);
		}
	}
}

void KER_layer_collection_local_sync(ViewLayer *view_layer, const View3D *v3d) {
	if (no_resync) {
		return;
	}

	const unsigned int local_collections_uuid = v3d->local_collections_uuid;

	/* Reset flags and set the bases visible by default. */
	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		base->local_collections_bits &= ~local_collections_uuid;
	}

	LISTBASE_FOREACH(LayerCollection *, layer_collection, &view_layer->layer_collections) {
		layer_collection_local_sync(view_layer, layer_collection, local_collections_uuid, true);
	}
}

void KER_layer_collection_local_sync_all(const Main *main) {
	if (no_resync) {
		return;
	}

	LISTBASE_FOREACH(Scene *, scene, &main->scenes) {
		LISTBASE_FOREACH(ViewLayer *, view_layer, &scene->view_layers) {
			LISTBASE_FOREACH(Screen *, screen, &main->screens) {
				LISTBASE_FOREACH(ScrArea *, area, &screen->areabase) {
					if (area->spacetype != SPACE_VIEW3D) {
						continue;
					}
					View3D *v3d = area->spacedata.first;
					if (v3d->flag & V3D_LOCAL_COLLECTIONS) {
						KER_layer_collection_local_sync(view_layer, v3d);
					}
				}
			}
		}
	}
}

ROSE_STATIC void layer_collection_bases_show_recursive(ViewLayer *view_layer, LayerCollection *lc) {
	if ((lc->flag & LAYER_COLLECTION_EXCLUDE) == 0) {
		LISTBASE_FOREACH(CollectionObject *, cob, &lc->collection->objects) {
			Base *base = KER_view_layer_base_find(view_layer, cob->object);
			base->flag &= ~BASE_HIDDEN;
		}
	}
	LISTBASE_FOREACH(LayerCollection *, lc_iter, &lc->layer_collections) {
		layer_collection_bases_show_recursive(view_layer, lc_iter);
	}
}

ROSE_STATIC void layer_collection_bases_hide_recursive(ViewLayer *view_layer, LayerCollection *lc) {
	if ((lc->flag & LAYER_COLLECTION_EXCLUDE) == 0) {
		LISTBASE_FOREACH(CollectionObject *, cob, &lc->collection->objects) {
			Base *base = KER_view_layer_base_find(view_layer, cob->object);
			base->flag |= BASE_HIDDEN;
		}
	}
	LISTBASE_FOREACH(LayerCollection *, lc_iter, &lc->layer_collections) {
		layer_collection_bases_hide_recursive(view_layer, lc_iter);
	}
}

void KER_layer_collection_set_visible(ViewLayer *view_layer, LayerCollection *lc, const bool visible, const bool hierarchy) {
	if (hierarchy) {
		if (visible) {
			layer_collection_flag_unset_recursive(lc, LAYER_COLLECTION_HIDE);
			layer_collection_bases_show_recursive(view_layer, lc);
		}
		else {
			layer_collection_flag_set_recursive(lc, LAYER_COLLECTION_HIDE);
			layer_collection_bases_hide_recursive(view_layer, lc);
		}
	}
	else {
		if (visible) {
			lc->flag &= ~LAYER_COLLECTION_HIDE;
		}
		else {
			lc->flag |= LAYER_COLLECTION_HIDE;
		}
	}
}

ROSE_STATIC void layer_collection_flag_recursive_set(LayerCollection *lc, const int flag, const bool value, const bool restore_flag) {
	if (flag == LAYER_COLLECTION_EXCLUDE) {
		/* For exclude flag, we remember the state the children had before
		 * excluding and restoring it when enabling the parent collection again. */
		if (value) {
			if (restore_flag) {
				SET_FLAG_FROM_TEST(lc->flag, (lc->flag & LAYER_COLLECTION_EXCLUDE), LAYER_COLLECTION_PREVIOUSLY_EXCLUDED);
			}
			else {
				lc->flag &= ~LAYER_COLLECTION_PREVIOUSLY_EXCLUDED;
			}

			lc->flag |= flag;
		}
		else {
			if (!(lc->flag & LAYER_COLLECTION_PREVIOUSLY_EXCLUDED)) {
				lc->flag &= ~flag;
			}
		}
	}
	else {
		SET_FLAG_FROM_TEST(lc->flag, value, flag);
	}

	LISTBASE_FOREACH(LayerCollection *, nlc, &lc->layer_collections) {
		layer_collection_flag_recursive_set(nlc, flag, value, true);
	}
}

/** \} */

typedef struct LayerCollectionResync {
	struct LayerCollectionResync *prev, *next, *queue_next;

	LayerCollection *layer;
	Collection *collection;

	struct LayerCollectionResync *parent_layer_resync;
	ListBase children_layer_resync;

	bool is_usable;
	bool is_valid_as_parent;
	bool is_valid_as_child;
	bool is_used;
} LayerCollectionResync;

ROSE_STATIC LayerCollectionResync *layer_collection_resync_create_recurse(LayerCollectionResync *parent_layer_resync, LayerCollection *layer, MemPool *mempool) {
	LayerCollectionResync *layer_resync = LIB_memory_pool_calloc(mempool);

	layer_resync->layer = layer;
	layer_resync->collection = layer->collection;
	layer_resync->parent_layer_resync = parent_layer_resync;
	if (parent_layer_resync != NULL) {
		LIB_addtail(&parent_layer_resync->children_layer_resync, layer_resync);
	}

	layer_resync->is_usable = (layer->collection != NULL);
	layer_resync->is_valid_as_child = layer_resync->is_usable && (parent_layer_resync == NULL || (parent_layer_resync->is_usable && LIB_findptr(&parent_layer_resync->layer->collection->children, layer->collection, offsetof(CollectionChild, collection)) != NULL));

	if (layer_resync->is_valid_as_child) {
		layer_resync->is_used = parent_layer_resync != NULL ? parent_layer_resync->is_used : true;
	}
	else {
		layer_resync->is_used = false;
	}

	if (LIB_listbase_is_empty(&layer->layer_collections)) {
		layer_resync->is_valid_as_parent = layer_resync->is_usable;
	}
	else {
		LISTBASE_FOREACH(LayerCollection *, child_layer, &layer->layer_collections) {
			LayerCollectionResync *child_layer_resync = layer_collection_resync_create_recurse(layer_resync, child_layer, mempool);
			if (layer_resync->is_usable && child_layer_resync->is_valid_as_child) {
				layer_resync->is_valid_as_parent = true;
			}
		}
	}

	return layer_resync;
}

ROSE_STATIC LayerCollectionResync *layer_collection_resync_find(LayerCollectionResync *layer_resync, Collection *child_collection) {
	ROSE_assert(layer_resync->collection != child_collection);
	ROSE_assert(child_collection != NULL);

	LayerCollectionResync *current_layer_resync = NULL;
	LayerCollectionResync *root_layer_resync = layer_resync;
	LayerCollectionResync *queue_head = layer_resync, *queue_tail = layer_resync;

	layer_resync->queue_next = NULL;

	while (queue_head != NULL) {
		current_layer_resync = queue_head;
		queue_head = current_layer_resync->queue_next;

		if (current_layer_resync->collection == child_collection && (current_layer_resync->parent_layer_resync == layer_resync || (!current_layer_resync->is_used && !current_layer_resync->is_valid_as_child))) {
			/**
			 * This layer is a valid candidate, because its collection matches the seeked one, AND: 
			 *  - It is a direct child of the initial given parent ('unchanged hierarchy' case), OR
			 *  - It is not currently used, and not part of a valid hierarchy (sub-)chain.
			 */
			break;
		}

		LISTBASE_FOREACH(LayerCollectionResync *, child_layer_resync, &current_layer_resync->children_layer_resync) {
			queue_tail->queue_next = child_layer_resync;
			child_layer_resync->queue_next = NULL;
			queue_tail = child_layer_resync;
			if (queue_head == NULL) {
				queue_head = queue_tail;
			}
		}

		if (queue_head == NULL && root_layer_resync->parent_layer_resync != NULL) {
			LISTBASE_FOREACH(LayerCollectionResync *, siblind_layer_resync, &root_layer_resync->parent_layer_resync->children_layer_resync) {
				if (siblind_layer_resync == root_layer_resync) {
					continue;
				}
				queue_tail->queue_next = siblind_layer_resync;
				siblind_layer_resync->queue_next = NULL;
				queue_tail = siblind_layer_resync;

				if (queue_head == NULL) {
					queue_head = queue_tail;
				}
			}

			root_layer_resync = root_layer_resync->parent_layer_resync;
		}
		current_layer_resync = NULL;
	}

	return current_layer_resync;
}

ROSE_STATIC void layer_collection_resync_unused_layers_free(ViewLayer *view_layer, LayerCollectionResync *layer_resync) {
	LISTBASE_FOREACH(LayerCollectionResync *, child_layer_resync, &layer_resync->children_layer_resync) {
		layer_collection_resync_unused_layers_free(view_layer, child_layer_resync);
	}

	if (!layer_resync->is_used) {
		if (layer_resync->layer == view_layer->active_collection) {
			view_layer->active_collection = NULL;
		}

		MEM_freeN(layer_resync->layer);
		layer_resync->layer = NULL;
		layer_resync->collection = NULL;
		layer_resync->is_usable = false;
	}
}

ROSE_STATIC void layer_collection_objects_sync(ViewLayer *view_layer, LayerCollection *layer, ListBase *r_lb_new_object_bases, const short collection_restrict, const short layer_restrict, const ushort local_collections_bits) {
	/* No need to sync objects if the collection is excluded. */
	if ((layer->flag & LAYER_COLLECTION_EXCLUDE) != 0) {
		return;
	}

	LISTBASE_FOREACH(CollectionObject *, cob, &layer->collection->objects) {
		if (cob->object == NULL) {
			continue;
		}

		void **base_p;
		Base *base;
		if (LIB_ghash_ensure_p(view_layer->object_bases_hash, cob->object, &base_p)) {
			/* Move from old base list to new base list. Base might have already
			 * been moved to the new base list and the first/last test ensure that
			 * case also works. */
			base = *base_p;
			if (!ELEM(base, r_lb_new_object_bases->first, r_lb_new_object_bases->last)) {
				LIB_remlink(&view_layer->bases, base);
				LIB_addtail(r_lb_new_object_bases, base);
			}
		}
		else {
			/* Create new base. */
			base = object_base_new(cob->object);
			base->local_collections_bits = local_collections_bits;
			*base_p = base;
			LIB_addtail(r_lb_new_object_bases, base);
		}

		if ((collection_restrict & COLLECTION_HIDE_VIEWPORT) == 0) {
			base->flag_collection |= BASE_ENABLED_VIEWPORT;
			if ((layer_restrict & LAYER_COLLECTION_HIDE) == 0) {
				base->flag_collection |= BASE_VISIBLE_VIEWLAYER;
			}
			if (((collection_restrict & COLLECTION_HIDE_SELECT) == 0)) {
				base->flag_collection |= BASE_SELECTABLE;
			}
		}

		if ((collection_restrict & COLLECTION_HIDE_RENDER) == 0) {
			base->flag_collection |= BASE_ENABLED_RENDER;
		}

		layer->runtime_flag |= LAYER_COLLECTION_HAS_OBJECTS;
	}
}

ROSE_STATIC void layer_collection_sync(ViewLayer *view_layer, LayerCollectionResync *layer_resync, MemPool *layer_resync_mempool, ListBase *r_lb_new_object_bases, const short flag, const short crestrict, const short lrestrict, const unsigned int local_collection_bits) {
	/**
	 * This function assumes current 'parent' layer collection is already fully (re)synced and valid 
	 * regarding current Collection hierarchy.
	 *
	 * It will process all the children collections of the collection from the given 'parent' layer,
	 * re-use or create layer collections for each of them, and ensure orders also match.
	 *
	 * Then it will ensure that the objects owned by the given parent collection have a proper base.
	 *
	 * NOTE: This process is recursive.
	 */

	ListBase new_lb_layer = {NULL, NULL};

	ROSE_assert(layer_resync->is_used);

	LISTBASE_FOREACH(CollectionChild *, child, &layer_resync->collection->children) {
		Collection *child_collection = child->collection;

		LayerCollectionResync *child_layer_resync = layer_collection_resync_find(layer_resync, child_collection);
		if (child_layer_resync) {
			ROSE_assert(child_layer_resync->collection != NULL);
			ROSE_assert(child_layer_resync->layer != NULL);
			ROSE_assert(child_layer_resync->is_usable != false);

			if (child_layer_resync->is_used) {
				fprintf(stdout, "[Kernel] Found same existing LayerCollection for %s as child of %s.\n", child_collection->id.name, layer_resync->collection->id.name);
			}
			else {
				fprintf(stdout, "[Kernel] Found a valid unused LayerCollection %s as child of %s, re-using it.\n", child_collection->id.name, layer_resync->collection->id.name);
			}

			child_layer_resync->is_used = true;

			LIB_remlink(&child_layer_resync->parent_layer_resync->layer->layer_collections, child_layer_resync->layer);
			LIB_remlink(&new_lb_layer, child_layer_resync->layer);
		}
		else {
			LayerCollection *child_layer = layer_collection_add(&new_lb_layer, child_collection);

			child_layer->flag = flag;

			child_layer_resync = LIB_memory_pool_calloc(layer_resync_mempool);
			child_layer_resync->collection = child_collection;
			child_layer_resync->layer = child_layer;
			child_layer_resync->is_usable = true;
			child_layer_resync->is_used = true;
			child_layer_resync->is_valid_as_child = true;
			child_layer_resync->is_valid_as_parent = true;
			child_layer_resync->parent_layer_resync = layer_resync;

			LIB_addtail(&layer_resync->children_layer_resync, child_layer_resync);
		}

		LayerCollection *child_layer = child_layer_resync->layer;

		short child_local_collection_bits = local_collection_bits & child_layer->local_collections_bits;
		short child_crestrict = crestrict;
		short child_lrestrict = lrestrict;
		if (!(child_collection->flag & COLLECTION_IS_MASTER)) {
			child_crestrict |= child_collection->flag;
			child_lrestrict |= child_layer->flag;
		}

		layer_collection_sync(view_layer, child_layer_resync, layer_resync_mempool, r_lb_new_object_bases, child_layer->flag, child_crestrict, child_lrestrict, child_local_collection_bits);

		child_layer->runtime_flag = 0;
		if (child_layer->flag & LAYER_COLLECTION_EXCLUDE) {
			continue;
		}

		if (child_crestrict & COLLECTION_HIDE_VIEWPORT) {
			child_layer->runtime_flag |= LAYER_COLLECTION_HIDE_VIEWPORT;
		}
		if (((child_layer->runtime_flag & LAYER_COLLECTION_HIDE_VIEWPORT) == 0) && ((child_layer->runtime_flag & LAYER_COLLECTION_HIDE) == 0)) {
			child_layer->runtime_flag |= LAYER_COLLECTION_VISIBLE_VIEW_LAYER;
		}
	}

	layer_resync->layer->layer_collections = new_lb_layer;
	ROSE_assert(LIB_listbase_count(&layer_resync->collection->children) == LIB_listbase_count(&new_lb_layer));

	layer_collection_objects_sync(view_layer, layer_resync->layer, r_lb_new_object_bases, crestrict, lrestrict, local_collection_bits);
}

#ifndef NDEBUG
ROSE_STATIC bool view_layer_objects_base_cache_validate(ViewLayer *view_layer, LayerCollection *layer) {
	bool is_valid = true;

	if (layer == NULL) {
		layer = view_layer->layer_collections.first;
	}

	if ((layer->flag & LAYER_COLLECTION_EXCLUDE) == 0) {
		LISTBASE_FOREACH(CollectionObject *, cob, &layer->collection->objects) {
			if (cob->object == NULL) {
				continue;
			}
			if (LIB_ghash_lookup(view_layer->object_bases_hash, cob->object) == NULL) {
				fprintf(stderr, "[Kernel] Object '%s' from collection '%s' has no entry in view layer's object bases cache.\n", cob->object->id.name + 2, layer->collection->id.name + 2);
				is_valid = false;
				break;
			}
		}
	}

	if (is_valid) {
		LISTBASE_FOREACH(LayerCollection *, layer_child, &layer->layer_collections) {
			if (!view_layer_objects_base_cache_validate(view_layer, layer_child)) {
				is_valid = false;
				break;
			}
		}
	}

	return is_valid;
}
#else
ROSE_STATIC bool view_layer_objects_base_cache_validate(ViewLayer *view_layer, LayerCollection *layer) {
	return true;
}
#endif

void KER_layer_collection_sync(const Scene *scene, ViewLayer *view_layer) {
	if (no_resync) {
		return;
	}

	if (!scene->master_collection) {
		return;
	}

	if (LIB_listbase_is_empty(&view_layer->layer_collections)) {
		layer_collection_add(&view_layer->layer_collections, scene->master_collection);
	}

#ifndef NDEBUG
	LayerCollection *first_layer_collection = view_layer->layer_collections.first;
	ROSE_assert_msg(LIB_listbase_count(&view_layer->layer_collections) == 1, "ViewLayer's first level of children layer collection should always have exactly one item.");
	ROSE_assert_msg(first_layer_collection->collection == scene->master_collection, "ViewLayer's first layer collection should always be the one for the scene's master collection.");
#endif

	MEM_SAFE_FREE(view_layer->object_bases_array);

	if (!view_layer->object_bases_hash) {
		view_layer_bases_hash_create(view_layer, false);
	}

	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		base->flag &= ~g_base_collection_flags;
		base->flag_collection &= ~g_base_collection_flags;
	}

	MemPool *layer_resync_mempool = LIB_memory_pool_create(sizeof(LayerCollectionResync), 1024, 1024, ROSE_MEMPOOL_NOP);
	LayerCollectionResync *master_layer_resync = layer_collection_resync_create_recurse(NULL, view_layer->layer_collections.first, layer_resync_mempool);

	ListBase new_object_bases = {NULL, NULL};

	layer_collection_sync(view_layer, master_layer_resync, layer_resync_mempool, &new_object_bases, 0, 0, 0, ~0);
	layer_collection_resync_unused_layers_free(view_layer, master_layer_resync);
	master_layer_resync = NULL;

	LIB_memory_pool_destroy(layer_resync_mempool);

	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		if (view_layer->active == base) {
			view_layer->active = NULL;
		}
		if (base->object) {
			ROSE_assert(LIB_haslink(&new_object_bases, base) == false);
			ROSE_assert(LIB_findptr(&new_object_bases, base->object, offsetof(Base, object)) == NULL);
			LIB_ghash_remove(view_layer->object_bases_hash, base->object, NULL, NULL);
		}
	}

	LIB_freelistN(&view_layer->bases);
	view_layer->bases = new_object_bases;

	view_layer_objects_base_cache_validate(view_layer, NULL);

	LISTBASE_FOREACH(Base *, base, &view_layer->bases) {
		KER_base_eval_flags(base);
	}

	LayerCollection *active = view_layer->active_collection;
	if (active && layer_collection_hidden(view_layer, active)) {
		KER_layer_collection_activate_parent(view_layer, active);
	}
	else if (active == NULL) {
		view_layer->active_collection = view_layer->layer_collections.first;
	}
}

void KER_scene_collection_sync(const Scene *scene) {
	if (no_resync) {
		return;
	}

	LISTBASE_FOREACH(ViewLayer *, view_layer, &scene->view_layers) {
		KER_layer_collection_sync(scene, view_layer);
	}
}

void KER_main_collection_sync(const Main *main) {
	if (no_resync) {
		return;
	}

	LISTBASE_FOREACH(Scene *, scene, &main->scenes) {
		KER_scene_collection_sync(scene);
	}

	KER_layer_collection_local_sync_all(main);
}

void KER_main_collection_sync_remap(const Main *main) {
	if (no_resync) {
		return;
	}

	/* On remapping of object or collection pointers free caches. */
	/* TODO: try to make this faster */

	for (Scene *scene = main->scenes.first; scene; scene = scene->id.next) {
		LISTBASE_FOREACH(ViewLayer *, view_layer, &scene->view_layers) {
			MEM_SAFE_FREE(view_layer->object_bases_array);

			if (view_layer->object_bases_hash) {
				LIB_ghash_free(view_layer->object_bases_hash, NULL, NULL);
				view_layer->object_bases_hash = NULL;

				/* Directly re-create the mapping here, so that we can also deal with duplicates in
				 * `view_layer->object_bases` list of bases properly. This is the only place where such
				 * duplicates should be fixed, and not considered as a critical error. */
				view_layer_bases_hash_create(view_layer, true);
			}
		}

		KER_collection_object_cache_free(scene->master_collection);
		// DEG_id_tag_update_ex((Main *)main, &scene->master_collection->id, ID_RECALC_COPY_ON_WRITE);
		// DEG_id_tag_update_ex((Main *)main, &scene->id, ID_RECALC_COPY_ON_WRITE);
		fprintf(stdout, "[Kernel] Dependancy graph regarding the scene collection and scene need to be recalculated.\n");
	}

	for (Collection *collection = main->collections.first; collection; collection = collection->id.next) {
		KER_collection_object_cache_free(collection);
		// DEG_id_tag_update_ex((Main *)main, &collection->id, ID_RECALC_COPY_ON_WRITE);
		fprintf(stdout, "[Kernel] Dependancy graph regarding the main collections need to be recalculated.\n");
	}

	KER_main_collection_sync(main);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name View Layer (Copy)
 * \{ */

ROSE_STATIC void layer_collections_copy_data(ViewLayer *view_layer_dst, const ViewLayer *view_layer_src, ListBase *layer_collections_dst, const ListBase *layer_collections_src) {
	LIB_duplicatelist(layer_collections_dst, layer_collections_src);

	LayerCollection *layer_collection_dst = layer_collections_dst->first;
	const LayerCollection *layer_collection_src = layer_collections_src->first;

	while (layer_collection_dst != NULL) {
		layer_collections_copy_data(view_layer_dst, view_layer_src, &layer_collection_dst->layer_collections, &layer_collection_src->layer_collections);
		if (layer_collection_src == view_layer_src->active_collection) {
			view_layer_dst->active_collection = layer_collection_dst;
		}
		layer_collection_dst = layer_collection_dst->next;
		layer_collection_src = layer_collection_src->next;
	}
}

void KER_view_layer_copy_data(Scene *scene_dst, const Scene *scene_src, ViewLayer *view_layer_dst, const ViewLayer *view_layer_src, const int flag) {
	LIB_listbase_clear(&view_layer_dst->drawdata);
	view_layer_dst->object_bases_array = NULL;
	view_layer_dst->object_bases_hash = NULL;

	LIB_listbase_clear(&view_layer_dst->bases);
	LISTBASE_FOREACH(Base *, base_src, &view_layer_src->bases) {
		Base *base_dst = MEM_dupallocN(base_src);
		LIB_addtail(&view_layer_dst->bases, base_dst);
		if (view_layer_src->active == base_src) {
			view_layer_dst->active = base_dst;
		}
	}

	view_layer_dst->active_collection = NULL;
	layer_collections_copy_data(view_layer_dst, view_layer_src, &view_layer_dst->layer_collections, &view_layer_src->layer_collections);

	LayerCollection *lc_scene_dst = view_layer_dst->layer_collections.first;
	lc_scene_dst->collection = scene_dst->master_collection;
}

void KER_view_layer_rename(Main *main, Scene *scene, ViewLayer *view_layer, const char *newname) {
	char oldname[ARRAY_SIZE(view_layer->name)];

	LIB_strcpy(oldname, ARRAY_SIZE(view_layer->name), view_layer->name);
	LIB_uniquename(&scene->view_layers, view_layer, "ViewLayer", '.', offsetof(ViewLayer, name), sizeof(view_layer->name));
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Selection
 * \{ */

bool KER_layer_collection_objects_select(ViewLayer *view_layer, LayerCollection *lc, bool deselect) {
	if (lc->collection->flag & COLLECTION_HIDE_SELECT) {
		return false;
	}

	bool changed = false;

	if (!(lc->flag & LAYER_COLLECTION_EXCLUDE)) {
		LISTBASE_FOREACH(CollectionObject *, cob, &lc->collection->objects) {
			Base *base = KER_view_layer_base_find(view_layer, cob->object);

			if (base) {
				if (deselect) {
					if (base->flag & BASE_SELECTED) {
						base->flag &= ~BASE_SELECTED;
						changed = true;
					}
				}
				else {
					if ((base->flag & BASE_SELECTABLE) && !(base->flag & BASE_SELECTED)) {
						base->flag |= BASE_SELECTED;
						changed = true;
					}
				}
			}
		}
	}

	LISTBASE_FOREACH(LayerCollection *, iter, &lc->layer_collections) {
		changed |= KER_layer_collection_objects_select(view_layer, iter, deselect);
	}

	return changed;
}

bool KER_layer_collection_has_selected_objects(ViewLayer *view_layer, LayerCollection *lc) {
	if (lc->collection->flag & COLLECTION_HIDE_SELECT) {
		return false;
	}

	if (!(lc->flag & LAYER_COLLECTION_EXCLUDE)) {
		LISTBASE_FOREACH(CollectionObject *, cob, &lc->collection->objects) {
			Base *base = KER_view_layer_base_find(view_layer, cob->object);

			if (base && (base->flag & BASE_SELECTED) && (base->flag & BASE_VISIBLE_DEPSGRAPH)) {
				return true;
			}
		}
	}

	LISTBASE_FOREACH(LayerCollection *, iter, &lc->layer_collections) {
		if (KER_layer_collection_has_selected_objects(view_layer, iter)) {
			return true;
		}
	}

	return false;
}

bool KER_layer_collection_has_layer_collection(LayerCollection *lc_parent, LayerCollection *lc_child) {
	if (lc_parent == lc_child) {
		return true;
	}

	LISTBASE_FOREACH(LayerCollection *, lc_iter, &lc_parent->layer_collections) {
		if (KER_layer_collection_has_layer_collection(lc_iter, lc_child)) {
			return true;
		}
	}
	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object Visibility
 * \{ */

void KER_base_set_visible(Scene *scene, ViewLayer *view_layer, Base *base, bool extend) {
	if (!extend) {
		/* Make only one base visible. */
		LISTBASE_FOREACH(Base *, other, &view_layer->bases) {
			other->flag |= BASE_HIDDEN;
		}

		base->flag &= ~BASE_HIDDEN;
	}
	else {
		/* Toggle visibility of one base. */
		base->flag ^= BASE_HIDDEN;
	}

	KER_layer_collection_sync(scene, view_layer);
}

bool KER_base_is_visible(const View3D *v3d, const Base *base) {
	if ((base->flag & BASE_VISIBLE_DEPSGRAPH) == 0) {
		return false;
	}

	if (v3d == NULL) {
		return base->flag & BASE_VISIBLE_VIEWLAYER;
	}

	if (v3d->flag & V3D_LOCAL_COLLECTIONS) {
		return (v3d->local_collections_uuid & base->local_collections_bits) != 0;
	}

	return base->flag & BASE_VISIBLE_VIEWLAYER;
}

bool KER_object_is_visible_in_viewport(const View3D *v3d, const struct Object *ob) {
	ROSE_assert(v3d != NULL);

	if ((v3d->flag & V3D_LOCAL_COLLECTIONS) && ((v3d->local_collections_uuid & ob->runtime.local_collections_bits) == 0)) {
		return false;
	}

	/* If not using local collection the object may still be in a hidden collection. */
	if ((v3d->flag & V3D_LOCAL_COLLECTIONS) == 0) {
		return (ob->flag & BASE_VISIBLE_VIEWLAYER) != 0;
	}

	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Evaluation
 * \{ */

void KER_base_eval_flags(Base *base) {
	/* Apply collection flags. */
	base->flag &= ~g_base_collection_flags;
	base->flag |= (base->flag_collection & g_base_collection_flags);

	/* Apply object restrictions. */
	const int object_restrict = base->object->flag_visibility;
	if (object_restrict & OB_HIDE_VIEWPORT) {
		base->flag &= ~BASE_ENABLED_VIEWPORT;
	}
	if (object_restrict & OB_HIDE_RENDER) {
		base->flag &= ~BASE_ENABLED_RENDER;
	}
	if (object_restrict & OB_HIDE_SELECT) {
		base->flag &= ~BASE_SELECTABLE;
	}

	/* Apply viewport visibility by default. The dependency graph for render
	 * can change these again, but for tools we always want the viewport
	 * visibility to be in sync regardless if depsgraph was evaluated. */
	if (!(base->flag & BASE_ENABLED_VIEWPORT) || (base->flag & BASE_HIDDEN)) {
		base->flag &= ~(BASE_VISIBLE_DEPSGRAPH | BASE_VISIBLE_VIEWLAYER | BASE_SELECTABLE);
	}

	/* Deselect unselectable objects. */
	if (!(base->flag & BASE_SELECTABLE)) {
		base->flag &= ~BASE_SELECTED;
	}
}

static void layer_eval_view_layer(struct Depsgraph *depsgraph, struct Scene *UNUSED(scene), ViewLayer *view_layer) {
	/* Create array of bases, for fast index-based lookup. */
	const size_t num_object_bases = LIB_listbase_count(&view_layer->bases);
	MEM_SAFE_FREE(view_layer->object_bases_array);
	view_layer->object_bases_array = MEM_mallocN(sizeof(Base *) * num_object_bases, "view_layer->object_bases_array");
	size_t base_index;
	LISTBASE_FOREACH_INDEX(Base *, base, &view_layer->bases, base_index) {
		view_layer->object_bases_array[base_index] = base;
	}
}

void KER_layer_eval_view_layer_indexed(Depsgraph *depsgraph, Scene *scene, int view_layer_index) {
	ROSE_assert(view_layer_index >= 0);
	ViewLayer *view_layer = LIB_findlink(&scene->view_layers, view_layer_index);
	ROSE_assert(view_layer != NULL);
	layer_eval_view_layer(depsgraph, scene, view_layer);
}

/** \} */
