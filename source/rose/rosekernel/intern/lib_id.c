#include "MEM_alloc.h"

#include "DNA_ID.h"

#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_remap.h"
#include "KER_main.h"
#include "KER_namemap.h"

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
		if (idtype_info->init_data != NULL) {
			idtype_info->init_data(id);
		}
		return;
	}

	ROSE_assert_msg(0, "IDType Missing IDTypeInfo");
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

void KER_libblock_runtime_reset_remapping_status(struct ID *id) {
	id->runtime.remap.status = 0;
	id->runtime.remap.skipped_refcounted = 0;
}

void id_us_ensure_real(struct ID *id) {
	if(id) {
		const int limit = ID_FAKE_USERS(id);
		id->tag |= LIB_TAG_EXTRAUSER;
		if(id->users <= limit) {
			if(id->users < limit || ((id->users == limit) && (id->tag & LIB_TAG_EXTRAUSER_SET))) {
				ROSE_assert_msg(0, "ID user count error.");
			}
			id->users = limit + 1;
			id->tag |= LIB_TAG_EXTRAUSER_SET;
		}
	}
}

void id_us_clear_real(struct ID *id) {
	if(id && (id->tag & LIB_TAG_EXTRAUSER_SET)) {
		if(id->tag & LIB_TAG_EXTRAUSER_SET) {
			id->users--;
			ROSE_assert(id->users >= ID_FAKE_USERS(id));
		}
		id->tag &= ~(LIB_TAG_EXTRAUSER | LIB_TAG_EXTRAUSER_SET);
	}
}

void id_us_plus_no_lib(struct ID *id) {
	if(id) {
		if((id->tag & LIB_TAG_EXTRAUSER) && (id->tag & LIB_TAG_EXTRAUSER_SET)) {
			ROSE_assert(id->users >= 1);
			id->tag &= ~LIB_TAG_EXTRAUSER_SET;
		} else {
			ROSE_assert(id->users >= 0);
			id->users++;
		}
	}
}

void id_us_plus(struct ID *id) {
	ROSE_assert(!ID_IS_LINKED(id));
	if(id) {
		id_us_plus_no_lib(id);
	}
}

void id_us_min(struct ID *id) {
	if(id) {
		const int limit = ID_FAKE_USERS(id);
		
		if(id->users <= limit) {
			ROSE_assert_msg(0, "ID user decrement error.");
			id->users = limit;
		} else {
			id->users--;
		}
		
		if((id->users == limit) && (id->tag & LIB_TAG_EXTRAUSER)) {
			id_us_ensure_real(id);
		}
	}
}

void id_fake_user_set(struct ID *id) {
	if(id && !(id->flag & LIB_FAKEUSER)) {
		id->flag |= LIB_FAKEUSER;
		id_us_plus(id);
	}
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

	KER_namemap_ensure_valid(main->name_map, (size_t)KER_idtype_idcode_to_index(GS(id->name)), name);
	
	LIB_strncpy(id->name + 2, name, ARRAY_SIZE(id->name) - 2);
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

	if ((flag & LIB_ID_FREE_NO_USER_REFCOUNT) == 0) {
		/**
		 * We map each ID usage inside this data-block to NULL, 
		 * that way the designated free function for the data-block will not affect the refcount of the used data-blocks.
		 * 
		 * Usefull for example when we delete the whole #Main database, we might have already freed the used data-blocks.
		 */
		KER_libblock_relink_ex(main, id, NULL, NULL, ID_REMAP_SKIP_USER_CLEAR);
	}

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

void KER_id_free(struct Main *main, void *idv) {
	id_free(main, idv, 0, true);
}
