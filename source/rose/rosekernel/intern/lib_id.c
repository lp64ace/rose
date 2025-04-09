#include "MEM_guardedalloc.h"

#include "LIB_string.h"

#include "KER_idprop.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_remap.h"
#include "KER_main.h"

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
			KER_main_unlock(main);

			/** TODO: Ensure that the name given is unique! */
			LIB_strcpy(id->name + 2, ARRAY_SIZE(id->name) - 2, name);
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
	}

	KER_libblock_free_data(id, (flag & LIB_ID_FREE_NO_USER_REFCOUNT) == 0);

	if ((flag & LIB_ID_FREE_NO_MAIN) == 0) {
		KER_main_unlock(main);
	}

	MEM_freeN(id);

	return flag;
}

void KER_id_free_ex(Main *main, void *idv, int flag, bool use_flag_from_idtag) {
	id_free(main, idv, flag, use_flag_from_idtag);
}
void KER_id_free_us(struct Main *main, void *idv) {
	ID *id = (ID *)idv;

	id_us_min(id);

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
