#include "MEM_alloc.h"

#include "DNA_ID.h"

#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_main.h"

IDTypeInfo IDType_ID_LINK_PLACEHOLDER = {
	.id_code = ID_LINK_PLACEHOLDER,
	.id_filter = 0,
	.main_listbase_index = INDEX_ID_NULL,
	.struct_size = sizeof(ID),

	.name = "LinkPlaceholder",
	.name_plural = "Link Placeholders",

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = NULL,
};

size_t KER_libblock_get_alloc_info(short type, const char **name) {
	const IDTypeInfo *id_type = KER_idtype_get_info_from_idcode(type);

	if (id_type == NULL) {
		if (name != NULL) {
			*name = NULL;
		}
		return 0;
	}

	if (name != NULL) {
		*name = id_type->name;
	}
	return id_type->struct_size;
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

void *KER_libblock_alloc(Main *main, short type, const char *name, const int flag) {
	ROSE_assert((flag & LIB_ID_CREATE_NO_MAIN) != 0 || main != NULL);

	ID *id = KER_libblock_alloc_notest(type);

	if (id) {
		if ((flag & LIB_ID_CREATE_NO_MAIN) != 0) {
			id->tag |= LIB_TAG_NO_MAIN;
		}
		if ((flag & LIB_ID_CREATE_NO_USER_REFCOUNT) != 0) {
			id->tag |= LIB_TAG_NO_USER_REFCOUNT;
		}

		*((short *)id->name) = type;
		if ((flag & LIB_ID_CREATE_NO_USER_REFCOUNT) == 0) {
			id->users = 1;
		}
		if ((flag & LIB_ID_CREATE_NO_MAIN) == 0) {
			ListBase *lb = which_libbase(main, type);

			KER_main_lock(main);
			LIB_addtail(lb, id);
			KER_id_new_name_validate(main, id, name);
			KER_main_unlock(main);

			id->lib = main->lib;
		}
		else {
			LIB_strncpy(id->name + 2, name, sizeof(id->name) - 2);
		}
	}

	return id;
}

void KER_libblock_init_empty(struct ID *id) {
	const IDTypeInfo *idtype_info = KER_idtype_get_info_from_id(id);

	if (idtype_info != NULL) {
		if (idtype_info != NULL) {
			idtype_info->init_data(id);
		}
		return;
	}

	ROSE_assert_msg(0, "IDType Missing IDTypeInfo");
}

void KER_id_new_name_validate(struct Main *main, struct ID *id, const char *tname) {
	char name[ARRAY_SIZE(id->name) - 2];

	/** If no name given, use name of current ID. */
	if (tname == NULL) {
		tname = id->name + 2;
	}
	/** Make a copy of given name (tname args can be const). */
	LIB_strncpy(name, tname, ARRAY_SIZE(name));

	if (name[0] == '\0') {
		/** Disallow empty names. */
		LIB_strncpy(name, KER_idtype_idcode_to_name(GS(id->name)), ARRAY_SIZE(name));
	}

	LIB_strncpy(id->name + 2, name, sizeof(id->name) - 2);
}

void KER_libblock_free_data(ID *id, const bool do_id_user) {
}

void KER_libblock_free_datablock(ID *id, const int flag) {
	UNUSED_VARS(flag);

	const IDTypeInfo *idtype_info = KER_idtype_get_info_from_id(id);

	if (idtype_info != NULL) {
		if (idtype_info->free_data != NULL) {
			idtype_info->free_data(id);
		}
		return;
	}

	ROSE_assert_msg(0, "IDType Missing IDTypeInfo");
}

static void id_free(Main *main, void *idv, int flag, const bool use_flag_from_idtag) {
	ID *id = (ID *)idv;

	if (use_flag_from_idtag) {
		if ((id->tag & LIB_TAG_NO_MAIN) != 0) {
			flag |= LIB_ID_FREE_NO_MAIN;
		}
		else {
			flag &= ~LIB_ID_FREE_NO_MAIN;
		}

		if ((id->tag & LIB_TAG_NO_USER_REFCOUNT) != 0) {
			flag |= LIB_ID_FREE_NO_USER_REFCOUNT;
		}
		else {
			flag &= ~LIB_ID_FREE_NO_USER_REFCOUNT;
		}
	}

	ROSE_assert((flag & LIB_ID_FREE_NO_MAIN) != 0 || main != NULL);
	ROSE_assert((flag & LIB_ID_FREE_NO_MAIN) != 0 || (flag & LIB_ID_FREE_NO_USER_REFCOUNT) == 0);

	const short type = GS(id->name);

	KER_libblock_free_datablock(id, flag);

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
}

void KER_id_free_ex(struct Main *main, void *idv, int flag, bool use_flag_from_idtag) {
	id_free(main, idv, flag, use_flag_from_idtag);
}
