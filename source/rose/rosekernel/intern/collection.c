#include "MEM_guardedalloc.h"

#include "KER_collection.h"
#include "KER_idtype.h"
#include "KER_layer.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_scene.h"
#include "KER_object.h"

#include "DEG_depsgraph.h"

/* -------------------------------------------------------------------- */
/** \name Prototypes
 * \{ */

ROSE_STATIC bool collection_child_add(Collection *parent, Collection *child, const int flag, const bool us);
ROSE_STATIC bool collection_child_rem(Collection *parent, Collection *child);
ROSE_STATIC bool collection_object_add(Main *main, Collection *parent, Object *object, const int flag, const bool us);
ROSE_STATIC bool collection_object_rem(Main *main, Collection *parent, Object *object, const bool us);

ROSE_STATIC bool collection_has_child(Collection *parent, const Collection *collection);
ROSE_STATIC bool collection_has_child_recursive(Collection *parent, const Collection *collection);

ROSE_STATIC CollectionParent *collection_find_parent(Collection *collection, const Collection *parent);
ROSE_STATIC CollectionChild *collection_find_child(const Collection *parent, const Collection *child);
ROSE_STATIC CollectionChild *collection_find_child_recursive(const Collection *parent, const Collection *collection);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Collection Data-block
 * \{ */

ROSE_STATIC void collection_copy_data(Main *main, ID *id_dst, const ID *id_src, const int flag) {
	ROSE_assert((((const Collection *)id_src)->flag & COLLECTION_IS_MASTER) != 0);

	Collection *dst = (Collection *)id_dst;

	dst->flag &= ~COLLECTION_HAS_OBJECT_CACHE;
	dst->flag &= ~COLLECTION_HAS_OBJECT_CACHE_INSTANCED;

	LIB_listbase_clear(&dst->object_cache);
	LIB_listbase_clear(&dst->object_cache_instanced);

	LIB_listbase_clear(&dst->objects);
	LIB_listbase_clear(&dst->children);
	LIB_listbase_clear(&dst->parents);

	LISTBASE_FOREACH(const CollectionChild *, child, &((const Collection *)id_src)->children) {
		collection_child_add(dst, child->collection, flag, false);
	}
	LISTBASE_FOREACH(const CollectionObject *, cobj, &((const Collection *)id_src)->objects) {
		collection_object_add(main, dst, cobj->object, flag, false);
	}
}

ROSE_STATIC void collection_free_data(ID *id) {
	Collection *collection = (Collection *)id;

	LIB_freelistN(&collection->objects);
	LIB_freelistN(&collection->children);
	LIB_freelistN(&collection->parents);

	KER_collection_object_cache_free(collection);
}

ROSE_STATIC void collection_foreach_id(ID *id, struct LibraryForeachIDData *data) {
	Collection *collection = (Collection *)id;

	LISTBASE_FOREACH(CollectionObject *, object, &collection->objects) {
		KER_LIB_FOREACHID_PROCESS_IDSUPER(data, object->object, IDWALK_CB_USER);
	}
	LISTBASE_FOREACH(CollectionChild *, child, &collection->children) {
		KER_LIB_FOREACHID_PROCESS_IDSUPER(data, child->collection, IDWALK_CB_NEVER_SELF | IDWALK_CB_USER);
	}
	LISTBASE_FOREACH(CollectionParent *, parent, &collection->parents) {
		KER_LIB_FOREACHID_PROCESS_IDSUPER(data, parent->collection, IDWALK_CB_NEVER_SELF);
	}
}

/** This will return the #Scene that owns the #Collection. */
ROSE_STATIC ID *collection_owner_get(Main *main, ID *id) {
	if ((id->flag & ID_FLAG_EMBEDDED_DATA) == 0) {
		return id;
	}
	ROSE_assert((id->tag & ID_TAG_NO_MAIN) == 0);

	Collection *master_collection = (Collection *)id;
	ROSE_assert((master_collection->flag & COLLECTION_IS_MASTER) != 0);

	LISTBASE_FOREACH(Scene *, scene, &main->scenes) {
		if (scene->master_collection == master_collection) {
			return &scene->id;
		}
	}

	ROSE_assert_msg(0, "Collection with no owner, Critical Main inconsistency.");
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Collection Creation
 * \{ */

ROSE_STATIC Collection *collection_add(Main *main, Collection *parent, const char *name) {
	Collection *collection = KER_id_new(main, ID_GR, name);
	id_us_rem(&collection->id);
	if (parent) {
		collection_child_add(parent, collection, 0, true);
	}
	return collection;
}

Collection *KER_collection_add(Main *main, Collection *parent, const char *name) {
	Collection *collection = collection_add(main, parent, name);
	KER_main_collection_sync(main);
	return collection;
}

Collection *KER_collection_master_add() {
	Collection *master_collection = KER_libblock_alloc(NULL, ID_GR, "Colleciton", LIB_ID_CREATE_NO_MAIN);
	do {
		master_collection->id.flag |= ID_FLAG_EMBEDDED_DATA;
		master_collection->flag |= COLLECTION_IS_MASTER;
	} while (false);
	return master_collection;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Collection Deletion
 * \{ */

void KER_collection_free_data(Collection *collection) {
	KER_libblock_free_data(&collection->id, false);
	collection_free_data(&collection->id);
}

bool KER_collection_delete(Main *main, Collection *collection, bool hierarchy) {
	if ((collection->flag & COLLECTION_IS_MASTER) != 0) {
		ROSE_assert_msg(0, "Scene master collection can't be deleted");
		return false;
	}
	if (hierarchy) {
		CollectionObject *object = (CollectionObject *)collection->objects.first;
		while (object != NULL) {
			collection_object_rem(main, collection, object->object, true);
			object = (CollectionObject *)collection->objects.first;
		}
		CollectionChild *child = (CollectionChild *)collection->children.first;
		while (child != NULL) {
			KER_collection_delete(main, child->collection, hierarchy);
			child = (CollectionChild *)collection->children.first;
		}
	}
	else {
		LISTBASE_FOREACH(CollectionChild *, child, &collection->children) {
			LISTBASE_FOREACH(CollectionParent *, cparent, &collection->parents) {
				Collection *parent = cparent->collection;
				collection_child_add(parent, child->collection, 0, true);
			}
		}

		/** Note that we do not remove the children collections here, #KER_id_delete will deal with them */

		CollectionObject *object = (CollectionObject *)collection->objects.first;
		while (object != NULL) {
			LISTBASE_FOREACH(CollectionParent *, cparent, &collection->parents) {
				Collection *parent = cparent->collection;
				collection_object_add(main, parent, object->object, 0, true);
			}
			collection_object_rem(main, collection, object->object, true);
			object = (CollectionObject *)collection->objects.first;
		}
	}

	KER_id_free(main, collection);
	KER_main_collection_sync(main);
	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Collection Scene Membership
 * \{ */

bool KER_collection_is_in_scene(Collection *collection) {
	if (collection->flag & COLLECTION_IS_MASTER) {
		return true;
	}

	LISTBASE_FOREACH(CollectionParent *, cparent, &collection->parents) {
		if (KER_collection_is_in_scene(cparent->collection)) {
			return true;
		}
	}

	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Collection Children
 * \{ */

ROSE_STATIC bool collection_instance_find_recursive(Collection *collection, Collection *instance_collection) {
	LISTBASE_FOREACH(CollectionObject *, object, &collection->objects) {
		if (object->object != NULL && ELEM(object->object->instance_collection, instance_collection, collection)) {
			return true;
		}
	}
	LISTBASE_FOREACH(CollectionChild *, child, &collection->children) {
		if (child->collection != NULL && collection_instance_find_recursive(child->collection, instance_collection)) {
			return true;
		}
	}
	return false;
}

/** Returns true if we add #collection as a child of #new_ancestor there will be cycles. */
bool KER_collection_cycle_find(Collection *new_ancenstor, Collection *collection) {
	if (collection == new_ancenstor) {
		return true;
	}
	if (collection == NULL) {
		collection = new_ancenstor;
	}
	LISTBASE_FOREACH(CollectionParent *, parent, &new_ancenstor->parents) {
		if (KER_collection_cycle_find(parent->collection, collection)) {
			return true;
		}
	}
	return collection_instance_find_recursive(collection, new_ancenstor);
}

ROSE_STATIC bool collection_child_add(Collection *parent, Collection *collection, const int flag, const bool us) {
	if (collection_has_child(parent, collection)) {
		return false;
	}
	if (KER_collection_cycle_find(parent, collection)) {
		return false;
	}
	CollectionChild *child = MEM_mallocN(sizeof(CollectionChild), "CollectionChild");
	child->collection = collection;
	LIB_addtail(&parent->children, child);

	if ((flag & LIB_ID_CREATE_NO_MAIN) == 0) {
		CollectionParent *cparent = MEM_mallocN(sizeof(CollectionParent), "CollectionParent");
		cparent->collection = parent;
		LIB_addtail(&collection->parents, cparent);
	}

	if (us) {
		id_us_add(&collection->id);
	}

	KER_collection_object_cache_free(parent);
	return true;
}

ROSE_STATIC bool collection_child_rem(Collection *parent, Collection *collection) {
	CollectionChild *child = collection_find_child(parent, collection);
	if (child == NULL) {
		return false;
	}

	CollectionParent *cparent = collection_find_parent(collection, parent);
	LIB_remlink(&collection->parents, cparent);
	LIB_remlink(&parent->children, child);
	MEM_freeN(cparent);
	MEM_freeN(child);

	id_us_rem(&collection->id);

	KER_collection_object_cache_free(parent);
	return true;
}

ROSE_STATIC CollectionParent *collection_find_parent(Collection *collection, const Collection *parent) {
	return (CollectionParent *)LIB_findptr(&collection->parents, parent, offsetof(CollectionParent, collection));
}

ROSE_STATIC bool collection_has_child(Collection *parent, const Collection *collection) {
	return (LIB_findptr(&parent->children, collection, offsetof(CollectionChild, collection)) != NULL);
}
ROSE_STATIC bool collection_has_child_recursive(Collection *parent, const Collection *collection) {
	LISTBASE_FOREACH(CollectionChild *, child, &parent->children) {
		if (child->collection == collection) {
			return true;
		}
		if (collection_find_child_recursive(child->collection, collection)) {
			return true;
		}
	}
	return false;
}

ROSE_STATIC CollectionChild *collection_find_child(const Collection *parent, const Collection *collection) {
	return (CollectionChild *)LIB_findptr(&parent->children, collection, offsetof(CollectionChild, collection));
}
ROSE_STATIC CollectionChild *collection_find_child_recursive(const Collection *parent, const Collection *collection) {
	for (CollectionChild *child = (CollectionChild *)((&parent->children)->first), *ret = NULL; child; child = child->next) {
		if (child->collection == collection) {
			return child;
		}
		if (ret = collection_find_child_recursive(child->collection, collection)) {
			return ret;
		}
	}
	return NULL;
}

bool KER_collection_has_collection(const Collection *collection, const Collection *child) {
	return collection_find_child_recursive(collection, child);
}

bool KER_collection_child_add(Main *main, Collection *collection, Collection *child) {
	if (!collection_child_add(collection, child, 0, true)) {
		return false;
	}

	KER_main_collection_sync(main);
	return true;
}

bool KER_collection_child_rem(Main *main, Collection *collection, Collection *child) {
	if (!collection_child_rem(collection, child)) {
		return false;
	}

	KER_main_collection_sync(main);
	return true;
}

bool KER_collection_is_empty(const Collection *collection) {
	return LIB_listbase_is_empty(&collection->objects) && LIB_listbase_is_empty(&collection->children);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Object List Cache
 * \{ */

ROSE_STATIC void collection_object_cache_fill(ListBase *lb, Collection *collection, int parent_restrict, bool instances) {
	int child_restrict = collection->flag | parent_restrict;

	LISTBASE_FOREACH(CollectionObject *, cobj, &collection->objects) {
		Base *base = LIB_findptr(lb, cobj->object, offsetof(Base, object));

		if (!base) {
			base = MEM_callocN(sizeof(Base), "Object Base");
			base->object = cobj->object;
			LIB_addtail(lb, base);
			if (instances && cobj->object->instance_collection) {
				collection_object_cache_fill(lb, cobj->object->instance_collection, child_restrict, instances);
			}
		}

		if (((child_restrict & COLLECTION_HIDE_VIEWPORT) == 0)) {
			base->flag |= BASE_ENABLED_VIEWPORT;
		}
		if (((child_restrict & COLLECTION_HIDE_RENDER) == 0)) {
			base->flag |= BASE_ENABLED_RENDER;
		}
	}

	LISTBASE_FOREACH(CollectionChild *, child, &collection->children) {
		collection_object_cache_fill(lb, child->collection, child_restrict, instances);
	}
}

ListBase KER_collection_object_cache_get(Collection *collection) {
	if (!(collection->flag & COLLECTION_HAS_OBJECT_CACHE)) {
		static ThreadMutex cache_lock = ROSE_MUTEX_INITIALIZER;

		LIB_mutex_lock(&cache_lock);
		if (!(collection->flag & COLLECTION_HAS_OBJECT_CACHE)) {
			collection_object_cache_fill(&collection->object_cache, collection, 0, false);
			collection->flag |= COLLECTION_HAS_OBJECT_CACHE;
		}
		LIB_mutex_unlock(&cache_lock);
	}

	return collection->object_cache;
}

ListBase KER_collection_object_cache_instanced_get(Collection *collection) {
	if (!(collection->flag & COLLECTION_HAS_OBJECT_CACHE_INSTANCED)) {
		static ThreadMutex cache_lock = ROSE_MUTEX_INITIALIZER;

		LIB_mutex_lock(&cache_lock);
		if (!(collection->flag & COLLECTION_HAS_OBJECT_CACHE_INSTANCED)) {
			collection_object_cache_fill(&collection->object_cache_instanced, collection, 0, true);
			collection->flag |= COLLECTION_HAS_OBJECT_CACHE_INSTANCED;
		}
		LIB_mutex_unlock(&cache_lock);
	}

	return collection->object_cache_instanced;
}

ROSE_STATIC void collection_object_cache_free(Collection *collection) {
	collection->flag &= ~COLLECTION_HAS_OBJECT_CACHE;
	collection->flag &= ~COLLECTION_HAS_OBJECT_CACHE_INSTANCED;
	LIB_freelistN(&collection->object_cache);
	LIB_freelistN(&collection->object_cache_instanced);

	LISTBASE_FOREACH(CollectionParent *, parent, &collection->parents) {
		collection_object_cache_free(parent->collection);
	}
}

void KER_collection_object_cache_free(Collection *collection) {
	collection_object_cache_free(collection);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Collection Object Membership
 * \{ */

ROSE_STATIC void collection_tag_update_parent_recursive(Main *main, Collection *collection, const int flag) {
	if (collection->flag & COLLECTION_IS_MASTER) {
		return;
	}

	DEG_id_tag_update_ex(main, &collection->id, flag);

	LISTBASE_FOREACH(CollectionParent *, cparent, &collection->parents) {
		if (cparent->collection->flag & COLLECTION_IS_MASTER) {
			continue;
		}
		collection_tag_update_parent_recursive(main, cparent->collection, flag);
	}
}

bool KER_collection_has_object(Collection *collection, Object *object) {
	if (ELEM(NULL, collection, object)) {
		return false;
	}
	return (LIB_findptr(&collection->objects, object, offsetof(CollectionObject, object)) != NULL);
}

bool KER_collection_has_object_recursive(Collection *collection, Object *object) {
	if (ELEM(NULL, collection, object)) {
		return false;
	}
	const ListBase objects = KER_collection_object_cache_get(collection);
	return (LIB_findptr(&objects, object, offsetof(Base, object)) != NULL);
}

bool KER_collection_has_object_recursive_instanced(Collection *collection, Object *object) {
	if (ELEM(NULL, collection, object)) {
		return false;
	}
	const ListBase objects = KER_collection_object_cache_instanced_get(collection);
	return (LIB_findptr(&objects, object, offsetof(Base, object)) != NULL);
}

static Collection *collection_parent_editable_find_recursive(const ViewLayer *view_layer, Collection *collection) {
	if (view_layer == NULL || KER_view_layer_has_collection(view_layer, collection)) {
		return collection;
	}

	if (collection->flag & COLLECTION_IS_MASTER) {
		return NULL;
	}

	LISTBASE_FOREACH(CollectionParent *, collection_parent, &collection->parents) {
		if (view_layer != NULL && !KER_view_layer_has_collection(view_layer, collection_parent->collection)) {
			/* In case this parent collection is not in given view_layer, there is no point in
			 * searching in its ancestors either, we can skip that whole parenting branch. */
			continue;
		}
		return collection_parent->collection;
	}

	return NULL;
}

bool collection_object_add(Main *main, Collection *collection, Object *object, const int flag, const bool us) {
	if (object->instance_collection) {
		if (collection_find_child_recursive(object->instance_collection, collection) || object->instance_collection == collection) {
			return false;
		}
	}
	CollectionObject *cobject = (CollectionObject *)LIB_findptr(&collection->objects, object, offsetof(CollectionObject, object));
	if (cobject) {
		return false;
	}

	cobject = MEM_mallocN(sizeof(CollectionObject), "CollectionObject");
	cobject->object = object;
	LIB_addtail(&collection->objects, cobject);

	KER_collection_object_cache_free(collection);

	if (us && (flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0) {
		id_us_add(&object->id);
	}
	return true;
}

bool collection_object_rem(Main *main, Collection *collection, Object *object, const bool us) {
	CollectionObject *cobject = LIB_findptr(&collection->objects, object, offsetof(CollectionObject, object));
	if (cobject == NULL) {
		return false;
	}
	LIB_freelinkN(&collection->objects, cobject);
	KER_collection_object_cache_free(collection);

	if (us) {
		KER_id_free_us(main, object);
	}
	else {
		id_us_rem(&object->id);
	}

	collection_tag_update_parent_recursive(main, collection, 0);
	return true;
}

bool KER_collection_object_add_notest(Main *main, Collection *collection, Object *ob) {
	if (ob == NULL) {
		return false;
	}

	ROSE_assert(collection != NULL);
	if (collection == NULL) {
		return false;
	}

	if (!collection_object_add(main, collection, ob, 0, true)) {
		return false;
	}

	if (KER_collection_is_in_scene(collection)) {
		KER_main_collection_sync(main);
	}


	DEG_id_tag_update(&collection->id, ID_RECALC_GEOMETRY);

	return true;
}

bool KER_collection_viewlayer_object_add(Main *main, const ViewLayer *view_layer, Collection *collection, Object *ob) {
	if (collection == NULL) {
		return false;
	}

	collection = collection_parent_editable_find_recursive(view_layer, collection);

	if (collection == NULL) {
		return false;
	}

	return KER_collection_object_add_notest(main, collection, ob);
}

bool KER_collection_object_add(Main *main, Collection *collection, Object *ob) {
	return KER_collection_viewlayer_object_add(main, NULL, collection, ob);
}

bool KER_collection_object_rem(Main *main, Collection *collection, Object *ob, bool free_us) {
	if (ELEM(NULL, collection, ob)) {
		return false;
	}

	if (!collection_object_rem(main, collection, ob, free_us)) {
		return false;
	}

	if (KER_collection_is_in_scene(collection)) {
		KER_main_collection_sync(main);
	}

	DEG_id_tag_update(&collection->id, ID_RECALC_GEOMETRY);

	return true;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Collection Data-block Definition
 * \{ */

IDTypeInfo IDType_ID_GR = {
	.idcode = ID_GR,

	.filter = FILTER_ID_GR,
	.depends = 0,
	.index = INDEX_ID_GR,
	.size = sizeof(Collection),

	.name = "Collection",
	.name_plural = "Collections",

	.flag = IDTYPE_FLAGS_NO_ANIMDATA,

	.init_data = NULL,
	.copy_data = collection_copy_data,
	.free_data = collection_free_data,

	.foreach_id = collection_foreach_id,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
