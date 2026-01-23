#include "MEM_guardedalloc.h"

#include "LIB_string.h"

#include "KER_anim_data.h"
#include "KER_idprop.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_remap.h"
#include "KER_lib_query.h"
#include "KER_main.h"
#include "KER_main_id_name_map.h"
#include "KER_main_name_map.h"

/* -------------------------------------------------------------------- */
/** \name Datablock DrawData
 * 
 * These draw data are not always used, only specific engines use them to 
 * support rarely used operations, they are not the same as the draw data 
 * that the #Mesh structure has in #MeshRuntime since they are used by all
 * engines!
 * 
 * \note They are currently used to handle modifiers running on the device.
 * \{ */

typedef struct IdDdtTempalte {
	ID id;
	AnimData *adt;
	DrawDataList drawdata;
} IdDdtTempalte;

ROSE_INLINE bool id_can_have_draw_data(const ID *id) {
	const IDTypeInfo *info = KER_idtype_get_info_from_id(id);

	if (info == NULL) {
		return false;
	}

	/** Cannot have draw data without having animation data, see #IdDdtTemplate. */
	ROSE_assert((info->flag & IDTYPE_FLAGS_DRAWDATA) == 0 || (info->flag & IDTYPE_FLAGS_NO_ANIMDATA) == 0);

	return (info->flag & IDTYPE_FLAGS_DRAWDATA) != 0;
}

DrawDataList *KER_drawdatalst_get(ID *id) {
	if (id_can_have_draw_data(id)) {
		IdDdtTempalte *idt = (IdDdtTempalte *)id;
		return &idt->drawdata;
	}
	return NULL;
}

DrawData *KER_drawdata_ensure(ID *id, struct DrawEngineType *engine, size_t size, DrawDataInitCb init_cb, DrawDataFreeCb free_cb) {
	ROSE_assert(size > sizeof(DrawData) && id_can_have_draw_data(id));

	DrawData *dd = KER_drawdata_get(id, engine);
	if (dd) {
		return dd;
	}

	DrawDataList *drawdata = KER_drawdatalst_get(id);

	dd = MEM_callocN(size, "DrawData");
	dd->engine = engine;
	dd->free = free_cb;
	/** Perform user side initialzation if needed. */
	if (init_cb) {
		init_cb(dd);
	}
	LIB_addtail((ListBase *)drawdata, dd);

	return dd;
}

DrawData *KER_drawdata_get(ID *id, struct DrawEngineType *engine) {
	DrawDataList *drawdata = KER_drawdatalst_get(id);

	if (drawdata == NULL) {
		return NULL;
	}

	LISTBASE_FOREACH(DrawData *, dd, drawdata) {
		if (dd->engine == engine) {
			return dd;
		}
	}

	return NULL;
}

void KER_drawdata_free(ID *id) {
	DrawDataList *drawdata = KER_drawdatalst_get(id);

	if (drawdata == NULL) {
		return;
	}

	LISTBASE_FOREACH(DrawData *, dd, drawdata) {
		if (dd->free) {
			dd->free(dd);
		}
	}

	LIB_freelistN((ListBase *)drawdata);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Creation
 * \{ */

size_t KER_libblock_get_alloc_info(short type, const char **r_name) {
	const IDTypeInfo *info = KER_idtype_get_info_from_idcode(type);

	if (info == NULL) {
		if (r_name) {
			*r_name = NULL;
		}
		return 0;
	}

	if (r_name) {
		*r_name = info->name;
	}
	return info->size;
}

void *KER_libblock_alloc_notest(short type) {
	const char *name;
	size_t size = KER_libblock_get_alloc_info(type, &name);
	if (size != 0) {
		return MEM_callocN(size, name);
	}
	ROSE_assert_msg(0, "Request to allocate unknown data type");
	return NULL;
}

void *KER_libblock_alloc(Main *main, short type, const char *name, int flag) {
	/**
	 * Main data-base is allowed to be NULL only if #LIB_ID_CREATE_NO_MAIN is specified.
	 */
	ROSE_assert((flag & LIB_ID_CREATE_NO_MAIN) != 0 || main != NULL);

	ID *id = KER_libblock_alloc_notest(type);

	if (id) {
		if ((flag & LIB_ID_CREATE_NO_MAIN) != 0) {
			id->tag |= ID_TAG_NO_MAIN;
		}
		if ((flag & LIB_ID_CREATE_NO_USER_REFCOUNT) != 0) {
			id->tag |= ID_TAG_NO_USER_REFCOUNT;
		}

		*((short *)id->name) = type;
		if ((flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0) {
			id->user = 1;
		}

		if ((flag & LIB_ID_CREATE_NO_MAIN) == 0) {
			ListBase *lb = which_libbase(main, type);

			id->lib = NULL;

			KER_main_lock(main);
			LIB_addtail(lb, id);
			KER_id_new_name_validate(main, lb, id, name);
			KER_main_unlock(main);

			if (main->curlib) {
				id->lib = main->curlib;
			}
		}
		else {
			LIB_strcpy(id->name + 2, ARRAY_SIZE(id->name) - 2, name);
			id->lib = NULL;
		}
	}

	return id;
}

void KER_libblock_init_empty(ID *id) {
	const IDTypeInfo *idtype_info = KER_idtype_get_info_from_id(id);

	if (idtype_info != NULL) {
		if (idtype_info->init_data != NULL) {
			idtype_info->init_data(id);
		}
		return;
	}

	ROSE_assert_msg(0, "IDType missing IDTypeInfo");
}

void *KER_id_new(struct Main *main, short type, const char *name) {
	ROSE_assert(main != NULL);

	if (name == NULL) {
		name = KER_idtype_idcode_to_name(type);
	}

	ID *id = KER_libblock_alloc(main, type, name, 0);
	KER_libblock_init_empty(id);

	return id;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Copy
 * \{ */

void KER_libblock_copy_ex(Main *main, const ID *id, ID **new_id_p, int flag) {
	ID *new_id = *new_id_p;

	ROSE_assert((flag & LIB_ID_CREATE_NO_MAIN) != 0 || main != NULL);

	if (!new_id) {
		new_id = KER_libblock_alloc(main, GS(id->name), KER_id_name(id), flag);
	}
	ROSE_assert(new_id != NULL);

	const size_t id_size = KER_libblock_get_alloc_info(GS(id->name), NULL);
	const size_t id_offset = sizeof(ID);

	if (id_size > id_offset) {
		const char *cp = (const char *)id;
		char *cpn = (char *)new_id;

		memcpy(cpn + id_offset, cp + id_offset, id_size - id_offset);
	}

	const int copy_flag = flag | LIB_ID_CREATE_NO_USER_REFCOUNT;

	if (id->properties) {
		new_id->properties = IDP_DuplicateProperty_ex(id->properties, copy_flag);
	}

	 if (id_can_have_animdata(new_id)) {
		IdAdtTemplate *iat = (IdAdtTemplate *)new_id;

		/* the duplicate should get a copy of the animdata */
		if ((flag & LIB_ID_COPY_NO_ANIMDATA) == 0) {
			/**
			 * Note that even though horrors like root node-trees are not in main, 
			 * the actions they use in their anim data *are* in main...
			 * super-mega-hooray.
			 */

			iat->adt = KER_animdata_copy_ex(main, iat->adt, copy_flag);
		}
		else {
			iat->adt = NULL;
		}
	}

	 if (flag & LIB_ID_COPY_ID_NEW_SET) {
		ID_NEW_SET(id, new_id);
	}

	*new_id_p = new_id;
}

void *KER_libblock_copy(Main *main, const ID *id) {
	ID *new_id = NULL;
	KER_libblock_copy_ex(main, id, &new_id, 0);
	return new_id;
}

typedef struct IDCopyLibManagementData {
	const ID *id_src;
	ID *id_dst;
	int flag;
} IDCopyLibManagementData;

ROSE_INLINE int id_copy_libmanagement_cb(LibraryIDLinkCallbackData *cb_data) {
	ID **id_pointer = cb_data->self_ptr;
	ID *id = *id_pointer;

	const int cb_flag = cb_data->cb_flag;
	IDCopyLibManagementData *data = (IDCopyLibManagementData *)(cb_data->user_data);

	/* Remap self-references to new copied ID. */
	if (id == data->id_src) {
		/* We cannot use self_id here, it is not *always* id_dst (thanks to confounded node-trees!). */
		id = *id_pointer = data->id_dst;
	}

	/* Increase used IDs refcount if needed and required. */
	if ((data->flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0 && (cb_flag & IDWALK_CB_USER)) {
		if ((data->flag & LIB_ID_CREATE_NO_MAIN) != 0) {
			ROSE_assert(cb_data->self_id->tag & ID_TAG_NO_MAIN);
			id_us_add(id);
		}
		else {
			id_us_add(id);
		}
	}

	return IDWALK_RET_NOP;
}

ID *KER_id_copy_ex(Main *main, const ID *id, ID **new_id_p, int flag) {
	ID *new_id = (new_id_p) ? *new_id_p : NULL;

	if (id == NULL) {
		return NULL;
	}

	const IDTypeInfo *idtype_info = KER_idtype_get_info_from_id(id);

	if (idtype_info != NULL) {
		if ((idtype_info->flag & IDTYPE_FLAGS_NO_COPY) != 0) {
			return NULL;
		}

		KER_libblock_copy_ex(main, id, &new_id, flag);

		if (idtype_info->copy_data != NULL) {
			idtype_info->copy_data(main, new_id, id, flag);
		}
	}
	else {
		ROSE_assert_msg(0, "IDType Missing IDTypeInfo");
	}

	ROSE_assert_msg(new_id, "Could not get an allocated new ID to copy into");

	if (!new_id) {
		return NULL;
	}

	/* Update ID refcount, remap pointers to self in new ID. */

	IDCopyLibManagementData data = {
		.id_src = id,
		.id_dst = new_id,
		.flag = 0,
	};
	KER_library_foreach_ID_link(main, new_id, id_copy_libmanagement_cb, &data, 0);

	if (new_id_p != NULL) {
		*new_id_p = new_id;
	}

	return new_id;
}

void *KER_id_copy(Main *main, const ID *id) {
	return KER_id_copy_ex(main, id, NULL, 0);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Deletion
 * \{ */

void KER_libblock_free_datablock(ID *id, int flag) {
	const IDTypeInfo *idtype_info = KER_idtype_get_info_from_id(id);

	if (idtype_info != NULL) {
		if (idtype_info->free_data != NULL) {
			idtype_info->free_data(id);
		}
		return;
	}

	ROSE_assert_msg(0, "IDType missing IDTypeInfo");
}

void KER_libblock_free_data(ID *id, bool do_id_user) {
	if (id->properties) {
		IDP_FreePropertyContent_ex(id->properties, do_id_user);
		MEM_freeN(id->properties);
		id->properties = NULL;
	}

	KER_animdata_free(id, do_id_user);
	KER_drawdata_free(id);
}

ROSE_STATIC int id_free(Main *main, void *idv, int flag, bool use_flag_from_idtag) {
	ID *id = (ID *)(idv);

	if (use_flag_from_idtag) {
		if ((id->tag & ID_TAG_NO_MAIN) != 0) {
			flag |= LIB_ID_FREE_NO_MAIN;
		}
		else {
			flag &= ~LIB_ID_FREE_NO_MAIN;
		}

		if ((id->tag & ID_TAG_NO_USER_REFCOUNT) != 0) {
			flag |= LIB_ID_FREE_NO_USER_REFCOUNT;
		}
		else {
			flag &= ~LIB_ID_FREE_NO_USER_REFCOUNT;
		}
	}

	ROSE_assert((flag & LIB_ID_FREE_NO_MAIN) != 0 || main != NULL);
	ROSE_assert((flag & LIB_ID_FREE_NO_MAIN) != 0 || (flag & LIB_ID_FREE_NO_USER_REFCOUNT) == 0);

	const short type = GS(id->name);

	if ((flag & LIB_ID_FREE_NO_USER_REFCOUNT) == 0) {
		/** Remove any referenced ID's from us. */
		KER_libblock_relink_ex(main, id, NULL, NULL, ID_REMAP_SKIP_USER_CLEAR);
	}

	KER_libblock_free_datablock(id, flag);

	/* avoid notifying on removed data */
	if ((flag & LIB_ID_FREE_NO_MAIN) == 0) {
		KER_main_lock(main);
	}

	if ((flag & LIB_ID_FREE_NO_MAIN) == 0) {
		ListBase *lb = which_libbase(main, type);
		LIB_remlink(lb, id);
		KER_main_name_map_remove_id(main, id, KER_id_name(id));
	}

	KER_libblock_free_data(id, (flag & LIB_ID_FREE_NO_USER_REFCOUNT) == 0);

	if ((flag & LIB_ID_FREE_NO_MAIN) == 0) {
		KER_main_unlock(main);
	}

	if ((flag & LIB_ID_FREE_NO_USER_REFCOUNT) == 0) {
		/** Remove any references to us. */
		KER_libblock_remap(main, id, NULL, ID_REMAP_FORCE_INTERNAL_RUNTIME_POINTERS);
	}

	MEM_freeN(id);

	return flag;
}

void KER_id_free_ex(Main *main, void *idv, int flag, bool use_flag_from_idtag) {
	id_free(main, idv, flag, use_flag_from_idtag);
}

void KER_id_free_us(struct Main *main, void *idv) {
	ID *id = (ID *)idv;

	id_us_rem(id);

	if (id->user == 0) {
		KER_libblock_unlink(main, id);
		KER_id_free_ex(main, idv, 0, true);
	}
}

void KER_id_free(struct Main *main, void *idv) {
	id_free(main, idv, 0, true);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Reference Management
 * \{ */

ROSE_INLINE int id_us_min(struct ID *id) {
	/**
	 * If there is a fake user set,
	 * then the minimum valid value of the user reference counter is one.
	 *
	 * If no extra or fake users are set then the user reference counter can become zero.
	 */
	return ID_FAKE_USERS(id);
}

void id_us_add(struct ID *id) {
	if (id) {
		id->user++;
	}
}
void id_us_rem(struct ID *id) {
	if (id) {
		if (id->user > id_us_min(id)) {
			--id->user;
		}
		else {
			ROSE_assert_msg(0, "ID user decrement error");
			id->user = id_us_min(id);
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Common Utils
 * \{ */

bool KER_id_new_name_validate(Main *main, ListBase *lb, ID *id, const char *tname) {
	bool result = false;
	char name[ARRAY_SIZE(id->name) - 2];

	/* if no name given, use name of current ID
	 * else make a copy (tname args can be const) */
	if (tname == NULL) {
		tname = id->name + 2;
	}

	LIB_strcpy(name, ARRAY_SIZE(name), tname);

	if (name[0] == '\0') {
		/* Disallow empty names. */
		LIB_strcpy(name, ARRAY_SIZE(name), KER_idtype_idcode_to_name(GS(id->name)));
	}

	result = KER_main_name_map_get_unique_name(main, id, name);

	if (main->id_map) {
		KER_main_idmap_remove_id(main->id_map, id);
	}

	LIB_strcpy(id->name + 2, ARRAY_SIZE(id->name) - 2, name);
	
	if (main->id_map) {
		KER_main_idmap_insert_id(main->id_map, id);
	}

	return result;
}

const char *KER_id_name(const ID *id) {
	return id->name + 2;
}

/** \} */
