#include <stdlib.h>

#include "DNA_ID.h"

#include "LIB_assert.h"
#include "LIB_ghash.h"
#include "LIB_linklist.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"

enum {
	IDWALK_STOP = 1 << 0,
};

typedef struct LibraryForeachIDData {
	Main *main;

	ID *owner_id;
	ID *self_id;

	/** Flags controlling the behavior of the 'foreach id' looping code. */
	int flag;
	/** Generic flags to be passed to all callback calls for current processed data. */
	int cb_flag;
	/** Callback flags that are forbidden for all callback calls for current processed data. */
	int cb_flag_clear;

	/** Function to call for every ID pointer of current processed ata, and its opaque user data pointer. */
	LibraryIDLinkCallback callback;
	void *user_data;

	/** Store the returned value from the callback, to decide how to continue the processing of ID pointers for current data. */
	int status;

	GSet *ids_handled;
	LinkNode *ids_todo;
} LibraryForeachIDData;

bool KER_lib_query_foreachid_iter_stop(const struct LibraryForeachIDData *data) {
	return (data->status & IDWALK_STOP) != 0;
}

void KER_lib_query_foreachid_process(struct LibraryForeachIDData *data, struct ID **id_pp, int cb_flag) {
	if (KER_lib_query_foreachid_iter_stop(data)) {
		return;
	}

	const int flag = data->flag;
	ID *old_id = *id_pp;

	/* Update the callback flags with the ones defined (or forbidden) in `data` by the generic caller code. */
	cb_flag = ((cb_flag | data->cb_flag) & ~data->cb_flag_clear);

	LibraryIDLinkCallbackData callback_data;
	callback_data.user_data = data->user_data;
	callback_data.main = data->main;
	callback_data.owner_id = data->owner_id;
	callback_data.self_id = data->self_id;
	callback_data.id_pointer = id_pp;
	callback_data.cb_flag = cb_flag;
	const int callback_return = data->callback(&callback_data);
	if (flag & IDWALK_READONLY) {
		ROSE_assert(*(id_pp) == old_id);
	}
	if (old_id && (flag & IDWALK_RECURSE)) {
		if(LIB_gset_add((data)->ids_handled, old_id)) {
			if (!(callback_return & IDWALK_RET_STOP_RECURSION)) {
				LIB_linklist_prepend(&(data->ids_todo), old_id);
			}
		}
	}
	if (callback_return & IDWALK_RET_STOP_ITER) {
		data->status |= IDWALK_STOP;
	}
}

int KER_lib_query_foreachid_process_flags_get(LibraryForeachIDData *data) {
	return data->flag;
}
int KER_lib_query_foreachid_process_callback_flag_override(LibraryForeachIDData *data, int cb_flag, bool replace) {
	const int cb_flag_backup = data->cb_flag;
	if (replace) {
		data->cb_flag = cb_flag;
	}
	else {
		data->cb_flag |= cb_flag;
	}
	return cb_flag_backup;
}

static bool library_foreach_ID_link(Main *main, ID *owner_id, ID *id, LibraryIDLinkCallback callback, void *user_data, int flag, LibraryForeachIDData *inherit_data);

void KER_library_foreach_ID_link(struct Main *main, struct ID *id, LibraryIDLinkCallback cb, void *user_data, int flag) {
	library_foreach_ID_link(main, NULL, id, cb, user_data, flag, NULL);
}

static void library_foreach_ID_data_cleanup(LibraryForeachIDData *data) {
	if (data->ids_handled != NULL) {
		LIB_gset_free(data->ids_handled, NULL);
		LIB_linklist_free(data->ids_todo, NULL);
	}
}

static bool library_foreach_ID_link(Main *main, ID *owner_id, ID *id, LibraryIDLinkCallback callback, void *user_data, int flag, LibraryForeachIDData *inherit_data) {
	LibraryForeachIDData data;
	data.main = main;

	ROSE_assert(inherit_data == NULL || data.main == inherit_data->main);
	ROSE_assert((flag & (IDWALK_NO_ORIG_POINTERS_ACCESS | IDWALK_RECURSE)) != (IDWALK_NO_ORIG_POINTERS_ACCESS | IDWALK_RECURSE));

	if (flag & IDWALK_RECURSE) {
		flag |= IDWALK_READONLY;
		flag &= ~IDWALK_DO_INTERNAL_RUNTIME_POINTERS;

		data.ids_handled = LIB_gset_new(LIB_ghashutil_ptrhash, LIB_ghashutil_ptrcmp, "rose::kernel::foreachID_handled");
		data.ids_todo = NULL;

		LIB_gset_add(data.ids_handled, id);
	}
	else {
		data.ids_handled = NULL;
		data.ids_todo = NULL;
	}
	data.flag = flag;
	data.status = 0;
	data.callback = callback;
	data.user_data = user_data;

#define CALLBACK_INVOKE_ID(check_id, cb_flag) \
	{ \
		KER_lib_query_foreachid_process(&data, (ID **)&(check_id), (cb_flag)); \
		if (KER_lib_query_foreachid_iter_stop(&data)) { \
			library_foreach_ID_data_cleanup(&data); \
			return false; \
		} \
	} \
	((void)0)

#define CALLBACK_INVOKE(check_id_super, cb_flag) \
	{ \
		KER_lib_query_foreachid_process(&data, (ID **)&(check_id_super), (cb_flag)); \
		if (KER_lib_query_foreachid_iter_stop(&data)) { \
			library_foreach_ID_data_cleanup(&data); \
			return false; \
		} \
	} \
	((void)0)

	for (; id != NULL; id = (flag & IDWALK_RECURSE) ? LIB_linklist_pop(&data.ids_todo) : NULL) {
		data.self_id = id;
		data.owner_id = data.self_id;

		if (inherit_data == NULL) {
			data.cb_flag_clear = (id->tag & LIB_TAG_NO_USER_REFCOUNT) ? IDWALK_CB_USER | IDWALK_CB_USER_ONE : 0;
		}
		else {
			data.cb_flag = inherit_data->cb_flag;
			data.cb_flag_clear = inherit_data->cb_flag_clear;
		}

		if (flag & IDWALK_DO_LIBRARY_POINTER) {
			CALLBACK_INVOKE(id->lib, IDWALK_CB_NEVER_SELF);
		}

		if (flag & IDWALK_DO_INTERNAL_RUNTIME_POINTERS) {
			CALLBACK_INVOKE_ID(id->newid, IDWALK_CB_INTERNAL);
			CALLBACK_INVOKE_ID(id->orig_id, IDWALK_CB_INTERNAL);
		}

		const IDTypeInfo *id_type = KER_idtype_get_info_from_id(id);
		if (id_type->foreach_id != NULL) {
			id_type->foreach_id(id, &data);

			if (KER_lib_query_foreachid_iter_stop(&data)) {
				library_foreach_ID_data_cleanup(&data);
				return false;
			}
		}
	}

	library_foreach_ID_data_cleanup(&data);
	return true;

#undef CALLBACK_INVOKE_ID
#undef CALLBACK_INVOKE
}

/** TODO: Fix Me! */
bool KER_library_id_can_use_idtype(struct ID *owner_id, short id_type) {
	return true;
}
