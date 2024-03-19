#include "DNA_ID.h"

#include "MEM_alloc.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_remap.h"
#include "KER_lib_query.h"

#include "LIB_array.hh"
#include "LIB_map.hh"

using IDTypeFilter = uint64_t;

namespace rose::kernel::id::remapper {

struct IDRemapper {
private:
	Map<ID *, ID *> mappings;
	IDTypeFilter source_types = 0;

public:
	void clear() {
		mappings.clear();
		source_types = 0;
	}

	bool is_empty() const {
		return mappings.is_empty();
	}

	void add(ID *old_id, ID *new_id) {
		ROSE_assert(old_id != NULL);
		ROSE_assert(new_id == NULL || (GS(old_id->name) == GS(new_id->name)));
		mappings.add(old_id, new_id);
		ROSE_assert(KER_idtype_idcode_to_idfilter(GS(old_id->name)) != 0);
		source_types |= KER_idtype_idcode_to_idfilter(GS(old_id->name));
	}

	void add_overwrite(ID *old_id, ID *new_id) {
		ROSE_assert(old_id != NULL);
		ROSE_assert(new_id == NULL || (GS(old_id->name) == GS(new_id->name)));
		mappings.add_overwrite(old_id, new_id);
		ROSE_assert(KER_idtype_idcode_to_idfilter(GS(old_id->name)) != 0);
		source_types |= KER_idtype_idcode_to_idfilter(GS(old_id->name));
	}

	bool contains_mappings_for_any(IDTypeFilter filter) const {
		return (source_types & filter) != 0;
	}

	IDRemapperApplyResult get_mapping_result(ID *id, IDRemapperApplyOptions options, const ID *id_self) const {
		if (!mappings.contains(id)) {
			return ID_REMAP_RESULT_SOURCE_UNAVAILABLE;
		}
		const ID *new_id = mappings.lookup(id);
		if ((options & ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF) != 0 && id_self == new_id) {
			new_id = NULL;
		}
		if (new_id == NULL) {
			return ID_REMAP_RESULT_SOURCE_UNASSIGNED;
		}
		return ID_REMAP_RESULT_SOURCE_REMAPPED;
	}

	IDRemapperApplyResult apply(ID **r_id_ptr, IDRemapperApplyOptions options, ID *id_self) const {
		ROSE_assert(r_id_ptr != NULL);
		if (*r_id_ptr == NULL) {
			return ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE;
		}

		if (!mappings.contains(*r_id_ptr)) {
			return ID_REMAP_RESULT_SOURCE_UNAVAILABLE;
		}

		if (options & ID_REMAP_APPLY_UPDATE_REFCOUNT) {
			id_us_min(*r_id_ptr);
		}

		*r_id_ptr = mappings.lookup(*r_id_ptr);
		if (options & ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF && *r_id_ptr == id_self) {
			*r_id_ptr = NULL;
		}
		if (*r_id_ptr == NULL) {
			return ID_REMAP_RESULT_SOURCE_UNASSIGNED;
		}

		if (options & ID_REMAP_APPLY_UPDATE_REFCOUNT) {
			/** Do not handle LIB_TAG_INDIRECT/LIB_TAG_EXTERN here. */
			id_us_plus_no_lib(*r_id_ptr);
		}

		if (options & ID_REMAP_APPLY_ENSURE_REAL) {
			id_us_ensure_real(*r_id_ptr);
		}
		return ID_REMAP_RESULT_SOURCE_REMAPPED;
	}

	void iter(IDRemapperIterFunction func, void *user_data) const {
		for (auto item : mappings.items()) {
			func(item.key, item.value, user_data);
		}
	}
};

}  // namespace rose::kernel::id::remapper

/** \brief wrap CPP IDRemapper to a C handle. */
static IDRemapper *wrap(rose::kernel::id::remapper::IDRemapper *remapper) {
	return static_cast<IDRemapper *>(static_cast<void *>(remapper));
}

/** \brief wrap C handle to a CPP IDRemapper. */
static rose::kernel::id::remapper::IDRemapper *unwrap(IDRemapper *remapper) {
	return static_cast<rose::kernel::id::remapper::IDRemapper *>(static_cast<void *>(remapper));
}

/** \brief wrap C handle to a CPP IDRemapper. */
static const rose::kernel::id::remapper::IDRemapper *unwrap(const IDRemapper *remapper) {
	return static_cast<const rose::kernel::id::remapper::IDRemapper *>(static_cast<const void *>(remapper));
}

/* -------------------------------------------------------------------- */
/** \name IDRemapper
 * \{ */

IDRemapper *KER_id_remapper_create(void) {
	rose::kernel::id::remapper::IDRemapper *remapper = MEM_new<rose::kernel::id::remapper::IDRemapper>("rose::kernel::IDRemapper");

	return wrap(remapper);
}

void KER_id_remapper_clear(IDRemapper *id_remapper) {
	rose::kernel::id::remapper::IDRemapper *remapper = unwrap(id_remapper);
	remapper->clear();
}
void KER_id_remapper_free(IDRemapper *id_remapper) {
	rose::kernel::id::remapper::IDRemapper *remapper = unwrap(id_remapper);
	MEM_delete<rose::kernel::id::remapper::IDRemapper>(remapper);
}

void KER_id_remapper_add(IDRemapper *id_remapper, struct ID *old_id, struct ID *new_id) {
	rose::kernel::id::remapper::IDRemapper *remapper = unwrap(id_remapper);
	remapper->add(old_id, new_id);
}
void KER_id_remapper_add_override(IDRemapper *id_remapper, struct ID *old_id, struct ID *new_id) {
	rose::kernel::id::remapper::IDRemapper *remapper = unwrap(id_remapper);
	remapper->add_overwrite(old_id, new_id);
}

bool KER_id_remapper_is_empty(const IDRemapper *id_remapper) {
	const rose::kernel::id::remapper::IDRemapper *remapper = unwrap(id_remapper);
	return remapper->is_empty();
}

IDRemapperApplyResult KER_id_remapper_apply(const IDRemapper *id_remapper, struct ID **r_id_ptr, IDRemapperApplyOptions options) {
	ROSE_assert_msg((options & ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF) == 0, "ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF requires id_self parameter. Use `KER_id_remapper_apply_ex`.");
	return KER_id_remapper_apply_ex(id_remapper, r_id_ptr, options, nullptr);
}
IDRemapperApplyResult KER_id_remapper_apply_ex(const IDRemapper *id_remapper, struct ID **r_id_ptr, IDRemapperApplyOptions options, struct ID *id_self) {
	const rose::kernel::id::remapper::IDRemapper *remapper = unwrap(id_remapper);
	ROSE_assert_msg((options & ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF) == 0 || id_self != nullptr, "ID_REMAP_APPLY_WHEN_REMAPPING_TO_SELF requires id_self parameter.");
	return remapper->apply(r_id_ptr, options, id_self);
}
IDRemapperApplyResult KER_id_remapper_get_mapping_result(const IDRemapper *id_remapper, struct ID *id, IDRemapperApplyOptions options, const struct ID *id_self) {
	const rose::kernel::id::remapper::IDRemapper *remapper = unwrap(id_remapper);
	return remapper->get_mapping_result(id, options, id_self);
}

bool KER_id_remapper_has_mapping_for(const IDRemapper *id_remapper, uint64_t type_filter) {
	const rose::kernel::id::remapper::IDRemapper *remapper = unwrap(id_remapper);
	return remapper->contains_mappings_for_any(type_filter);
}

void KER_id_remapper_iter(const IDRemapper *id_remapper, IDRemapperIterFunction func, void *user_data) {
	const rose::kernel::id::remapper::IDRemapper *remapper = unwrap(id_remapper);
	remapper->iter(func, user_data);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Remapping Utils
 * \{ */

struct IDRemap {
	int type;

	Main *main;
	IDRemapper *id_remapper;

	ID *id_owner;
	int flag;
};

static void libblock_remap_reset_remapping_status_callback(ID *old_id, ID *new_id, void *) {
	KER_libblock_runtime_reset_remapping_status(old_id);
	if (new_id) {
		KER_libblock_runtime_reset_remapping_status(new_id);
	}
}

static void libblock_remap_data_preprocess(ID *id_owner, int remap_type, const IDRemapper *id_remapper) {
}

static void foreach_libblock_remap_callback_apply(ID *id_owner, ID *id_self, ID **id_ptr, IDRemap *id_remap_data, const IDRemapper *mappings, const IDRemapperApplyOptions options, const int cb_flag, const bool violates_never_null) {
	const bool skip_user_refcount = (id_remap_data->flag & ID_REMAP_SKIP_USER_REFCOUNT) != 0;
	const bool force_user_refcount = (id_remap_data->flag & ID_REMAP_FORCE_USER_REFCOUNT) != 0;
	ROSE_assert(!skip_user_refcount || !force_user_refcount);
	
	ID *old_id = *id_ptr;
	if(!violates_never_null) {
		KER_id_remapper_apply_ex(mappings, id_ptr, options, id_self);
	}
	
	ID *new_id = violates_never_null ? NULL : *id_ptr;
	
	if(new_id) {
		new_id->runtime.remap.status |= ID_REMAP_IS_LINKED_DIRECT;
	}
	
	if(skip_user_refcount) {
		return;
	}
	
	if(cb_flag & IDWALK_CB_USER) {
		if(force_user_refcount || (old_id->tag & LIB_TAG_NO_MAIN) == 0) {
			id_us_min(old_id);
		}
		if(new_id != nullptr && (force_user_refcount || (new_id->tag & LIB_TAG_NO_MAIN) == 0)) {
			id_us_plus_no_lib(new_id);
		}
	} else if (cb_flag & IDWALK_CB_USER_ONE) {
		id_us_ensure_real(new_id);
	}
}

static int foreach_libblock_remap_callback(LibraryIDLinkCallbackData *cb_data) {
	const int cb_flag = cb_data->cb_flag;

	ID *id_owner = cb_data->owner_id;
	ID *id_self = cb_data->self_id;
	ID **id_p = cb_data->id_pointer;

	IDRemap *id_remap_data = static_cast<IDRemap *>(cb_data->user_data);

	ROSE_assert(id_owner == id_remap_data->id_owner);

	if (*id_p == NULL) {
		return IDWALK_RET_NOP;
	}

	IDRemapper *id_remapper = id_remap_data->id_remapper;
	IDRemapperApplyOptions id_remapper_options = ID_REMAP_APPLY_DEFAULT;

	/* Used to cleanup all IDs used by a specific one. */
	if (id_remap_data->type == ID_REMAP_TYPE_CLEANUP) {
		/* Clearing existing instance to reduce potential lookup times for IDs referencing many other
		 * IDs. This makes sure that there will only be a single rule in the id_remapper. */
		KER_id_remapper_clear(id_remapper);
		KER_id_remapper_add(id_remapper, *id_p, nullptr);
	}

	if (cb_flag & IDWALK_CB_NEVER_SELF) {
		id_remapper_options |= ID_REMAP_APPLY_UNMAP_WHEN_REMAPPING_TO_SELF;
	}

	const IDRemapperApplyResult expected = KER_id_remapper_get_mapping_result(id_remapper, *id_p, id_remapper_options, id_self);
	if (ELEM(expected, ID_REMAP_RESULT_SOURCE_UNAVAILABLE, ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE)) {
		ROSE_assert_msg(id_remap_data->type == ID_REMAP_TYPE_REMAP, "Cleanup should always do unassign.");
		return IDWALK_RET_NOP;
	}

	const bool violates_never_null = ((cb_flag & IDWALK_CB_NEVER_NULL) && (expected == ID_REMAP_RESULT_SOURCE_UNASSIGNED) && (id_remap_data->flag & ID_REMAP_FORCE_NEVER_NULL_USAGE) == 0);
	const bool skip_reference = (id_remap_data->flag & ID_REMAP_SKIP_OVERRIDE_LIBRARY) != 0;
	const bool skip_never_null = (id_remap_data->flag & ID_REMAP_SKIP_NEVER_NULL_USAGE) != 0;
	
	if ((id_remap_data->flag & ID_REMAP_FLAG_NEVER_NULL_USAGE) && (cb_flag & IDWALK_CB_NEVER_NULL)) {
		id_owner->tag |= LIB_TAG_DOIT;
	}
	
	foreach_libblock_remap_callback_apply(id_owner, id_self, id_p, id_remap_data, id_remapper, id_remapper_options, cb_flag, violates_never_null);
	
	return IDWALK_RET_NOP;
}

static void libblock_remap_data(Main *main, ID *id, int remap_type, IDRemapper *id_remapper, const int remap_flags) {
	IDRemap id_remap_data;
	int foreach_id_flags;

	memset(&id_remap_data, 0, sizeof(IDRemap));

	foreach_id_flags = IDWALK_NOP;
	foreach_id_flags |= ((remap_flags & ID_REMAP_FORCE_UI_POINTERS) != 0) ? IDWALK_INCLUDE_UI : IDWALK_NOP;
	foreach_id_flags |= ((remap_flags & ID_REMAP_FORCE_INTERNAL_RUNTIME_POINTERS) != 0) ? IDWALK_DO_INTERNAL_RUNTIME_POINTERS : IDWALK_NOP;
	foreach_id_flags |= ((remap_flags & ID_REMAP_NO_ORIG_POINTERS_ACCESS) != 0) ? IDWALK_NO_ORIG_POINTERS_ACCESS : IDWALK_NOP;
	foreach_id_flags |= ((remap_flags & ID_REMAP_DO_LIBRARY_POINTERS) != 0) ? IDWALK_DO_LIBRARY_POINTER : IDWALK_NOP;

	id_remap_data.id_remapper = id_remapper;
	id_remap_data.type = remap_type;
	id_remap_data.main = main;
	id_remap_data.id_owner = nullptr;
	id_remap_data.flag = remap_flags;

	KER_id_remapper_iter(id_remapper, libblock_remap_reset_remapping_status_callback, nullptr);

	if (id) {
		libblock_remap_data_preprocess(id_remap_data.id_owner, remap_type, id_remapper);
		KER_library_foreach_ID_link(NULL, id, foreach_libblock_remap_callback, &id_remap_data, foreach_id_flags);
	}
	else {
	}
}

struct LibBlockRelinkMultipleUserData {
	Main *main;
	rose::Span<ID *> ids;
};

static void libblock_relink_foreach_idpair_cb(ID *old_id, ID *new_id, void *user_data) {
	LibBlockRelinkMultipleUserData *data = static_cast<LibBlockRelinkMultipleUserData *>(user_data);

	Main *main = data->main;
	const rose::Span<ID *> ids = data->ids;

	ROSE_assert(old_id != nullptr);
	ROSE_assert((new_id != nullptr) || GS(old_id->name) == GS(new_id->name));
	ROSE_assert(old_id != new_id);

	/* Post-Process should be done here. */
}

void KER_libblock_relink_multiple(Main *main, const rose::Span<ID *> ids, const int remap_type, IDRemapper *id_remapper, const int remap_flags) {
	ROSE_assert(remap_type == ID_REMAP_TYPE_REMAP || KER_id_remapper_is_empty(id_remapper));

	for (ID *id_iter : ids) {
		libblock_remap_data(main, id_iter, remap_type, id_remapper, remap_flags);
	}

	if (main == nullptr) {
		return;
	}

	switch (remap_type) {
		case ID_REMAP_TYPE_REMAP: {
			LibBlockRelinkMultipleUserData user_data = {main, ids};

			KER_id_remapper_iter(id_remapper, libblock_relink_foreach_idpair_cb, &user_data);
		} break;
		case ID_REMAP_TYPE_CLEANUP: {
			/* Post-Process should be done here. */
		} break;
	}

	// We should tag relations to be updated here!
}

void KER_libblock_relink_ex(Main *main, void *idv, void *old_idv, void *new_idv, int remap_flags) {
	/* Should be able to replace all _relink() functions (constraints, rigidbody, etc.) ? */

	ID *id = static_cast<ID *>(idv);
	ID *old_id = static_cast<ID *>(old_idv);
	ID *new_id = static_cast<ID *>(new_idv);
	rose::Span<ID *> ids = {id};

	IDRemapper *id_remapper = KER_id_remapper_create();
	int remap_type = ID_REMAP_TYPE_REMAP;

	ROSE_assert(id != nullptr);
	if (old_id != nullptr) {
		ROSE_assert((new_id == nullptr) || (GS(old_id->name) == GS(new_id->name)));
		ROSE_assert(old_id != new_id);
		KER_id_remapper_add(id_remapper, old_id, new_id);
	}
	else {
		ROSE_assert(new_id == nullptr);
		remap_type = ID_REMAP_TYPE_CLEANUP;
	}

	KER_libblock_relink_multiple(main, ids, remap_type, id_remapper, remap_flags);

	KER_id_remapper_free(id_remapper);
}

/* \} */

/* Keep Last */

const char *KER_id_remapper_result_string(const IDRemapperApplyResult result) {
	switch (result) {
		case ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE:
			return "not_mappable";
		case ID_REMAP_RESULT_SOURCE_UNAVAILABLE:
			return "unavailable";
		case ID_REMAP_RESULT_SOURCE_UNASSIGNED:
			return "unassigned";
		case ID_REMAP_RESULT_SOURCE_REMAPPED:
			return "remapped";
	}
	ROSE_assert_unreachable();
	return "";
}

static void id_remapper_print_item_cb(ID *old_id, ID *new_id, void * /*user_data*/) {
	if (old_id != nullptr && new_id != nullptr) {
		printf("  Remap %s(%p) to %s(%p)\n", old_id->name, old_id, new_id->name, new_id);
	}
	if (old_id != nullptr && new_id == nullptr) {
		printf("  Unassign %s(%p)\n", old_id->name, old_id);
	}
}

void KER_id_remapper_print(const IDRemapper *id_remapper) {
	printf("IDRemapper\n");
	KER_id_remapper_iter(id_remapper, id_remapper_print_item_cb, nullptr);
}
