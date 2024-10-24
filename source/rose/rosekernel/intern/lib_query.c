#include "KER_idprop.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_main.h"

#include "LIB_listbase.h"
#include "LIB_ghash.h"

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/* status */
enum {
	IDWALK_STOP = 1 << 0,
};

typedef struct LibraryForeachIDData {
	struct Main *main;
	struct ID *owner_id;
	struct ID *self_id;
	
	int flag;
	int cb_flag;
	int cb_flag_clear;
	
	/**
	 * Function to call for every ID pointers of current processed data, and its opaque user data 
	 * pointer.
	 */
	LibraryIDLinkCallback callback;
	/** The user_data associated with the callback. */
	void *user_data;
	
	/**
	 * Store the returned value from the callback, to decide how to continue the processing of ID 
	 * pointers for current data.
	 */
	int status;
	
	struct GSet *visited;
	struct ListBase todo;
} LibraryForeachIDData;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Helper Methods
 * \{ */

typedef struct TodoData {
	struct TodoData *prev, *next;
	
	struct ID *id;
} TodoData;

ROSE_INLINE void todo_push(LibraryForeachIDData *data, ID *id) {
	LIB_addtail(&data->todo, id);
}
ROSE_INLINE ID *todo_pop(LibraryForeachIDData *data) {
	if(!LIB_listbase_is_empty(&data->todo)) {
		return LIB_pophead(&data->todo);
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Foreach Methods
 * \{ */

bool KER_lib_query_foreachid_iter_stop(const LibraryForeachIDData *data) {
	return (data->status & IDWALK_STOP) != 0;
}

void KER_lib_query_foreachid_process(LibraryForeachIDData *data, ID **id_pp, int cb_flag) {
	if (KER_lib_query_foreachid_iter_stop(data)) {
		return;
	}
	
	const int flag = data->flag;
	ID *old_id = *id_pp;
	
	/**
	 * Update the callback flags with the ones defined (or forbidden) in `data` by the generic 
	 * caller code.
	 */
	cb_flag = ((cb_flag | data->cb_flag) & ~data->cb_flag_clear);
	
	LibraryIDLinkCallbackData callback_data;
	memset(&callback_data, 0, sizeof(LibraryIDLinkCallbackData));
	callback_data.user_data = data->user_data;
	callback_data.main = data->main;
	callback_data.owner_id = data->owner_id;
	callback_data.self_id = data->self_id;
	callback_data.self_ptr = id_pp;
	callback_data.cb_flag = cb_flag;
	const int callback_return = data->callback(&callback_data);
	
	if (flag & IDWALK_READONLY) {
		ROSE_assert(*(id_pp) == old_id);
	}
	else {
		ROSE_assert_msg((callback_return & (IDWALK_RET_STOP_ITER | IDWALK_RET_STOP_RECURSION)) == 0, "Iteration over ID usages should not be interrupted by the callback in non-readonly cases");
	}
	
	if (old_id && (flag & IDWALK_RECURSE)) {
		if (LIB_gset_add((data)->visited, old_id)) {
			if (!(callback_return & IDWALK_RET_STOP_RECURSION)) {
				todo_push(data, old_id);
			}
		}
	}
	if (callback_return & IDWALK_RET_STOP_ITER) {
		data->status |= IDWALK_STOP;
	}
}

int KER_lib_query_foreachid_process_flags_get(const LibraryForeachIDData *data) {
	return data->flag;
}
Main *KER_lib_query_foreachid_process_main_get(const LibraryForeachIDData *data) {
	return data->main;
}

int KER_lib_query_foreachid_process_callback_flag_override(LibraryForeachIDData *data, int cb_flag, bool do_replace) {
	const int cb_flag_backup = data->cb_flag;
	if (do_replace) {
		data->cb_flag = cb_flag;
	}
	else {
		data->cb_flag |= cb_flag;
	}
	return cb_flag_backup;
}

ROSE_STATIC void library_foreach_ID_data_cleanup(LibraryForeachIDData *data) {
	if (data->visited != NULL) {
		LIB_gset_free(data->visited, NULL);
		LIB_freelistN(&data->todo);
	}
}

void KER_lib_query_idpropertiesForeachIDLink_callback(IDProperty *id_prop, void *user_data) {
	ROSE_assert(id_prop->type == IDP_ID);

	LibraryForeachIDData *data = (LibraryForeachIDData *)user_data;
	const int cb_flag = IDWALK_CB_USER;
	/** Idealy KER_LIB_FOREACHID_PROCESS_ID would be used here but there is an issue with GCC. */
	KER_lib_query_foreachid_process(data, (ID **)&(id_prop->data.pointer), cb_flag);
	if (KER_lib_query_foreachid_iter_stop(data)) {
		return;
	}
}

ROSE_STATIC bool library_foreach_ID_link(Main *main, ID *owner_id, ID *id, LibraryIDLinkCallback callback, void *user_data, int flag, LibraryForeachIDData *inherit_data) {
	LibraryForeachIDData data;
	memset(&data, 0, sizeof(LibraryForeachIDData));
	data.main = main;
	
	ROSE_assert(inherit_data == NULL || data.main == inherit_data->main);
	if (flag & IDWALK_RECURSE) {
		/* For now, recursion implies read-only, and no internal pointers. */
		flag |= IDWALK_READONLY;
		flag &= ~IDWALK_DO_INTERNAL_RUNTIME_POINTERS;
		
		/**
		 * NOTE: This function itself should never be called recursively when IDWALK_RECURSE is set,
		 * see also comments in #BKE_library_foreach_ID_embedded.
		 * This is why we can always create this data here, and do not need to try and re-use it from
		 * `inherit_data`.
		 */
		data.visited = LIB_gset_ptr_new("library_foreach_ID_link::visited");
		LIB_listbase_clear(&data.todo);
		
		LIB_gset_add(data.visited, id);
	}
	else {
		data.visited = NULL;
	}
	
	data.flag = flag;
	data.status = 0;
	data.callback = callback;
	data.user_data = user_data;
	
#define CALLBACK_INVOKE_ID(check_id, cb_flag)								   \
	{																		   \
		KER_lib_query_foreachid_process(&data, (ID **)&(check_id), (cb_flag)); \
		if (KER_lib_query_foreachid_iter_stop(&data)) {						   \
			library_foreach_ID_data_cleanup(&data);							   \
			return false;													   \
		}																	   \
	}																		   \
	((void)0)

#define CALLBACK_INVOKE(check_id_super, cb_flag)									 \
	{																				 \
		KER_lib_query_foreachid_process(&data, (ID **)&(check_id_super), (cb_flag)); \
		if (KER_lib_query_foreachid_iter_stop(&data)) {								 \
			library_foreach_ID_data_cleanup(&data);									 \
			return false;															 \
		}																			 \
	}																				 \
	((void)0)
	
	for (; id != NULL; id = (flag & IDWALK_RECURSE) ? todo_pop(&data) : NULL, owner_id = NULL) {
		data.self_id = id;
		/* owner ID is same as self ID, except for embedded ID case. */
		if(id->flag & ID_FLAG_EMBEDDED_DATA) {
			data.owner_id = owner_id;
		}
		else {
			ROSE_assert(ELEM(owner_id, NULL, id));
			data.owner_id = id;
		}
		
		if (inherit_data == NULL) {
			data.cb_flag = 0;
			/* When an ID is defined as not reference-counting its ID usages, it should never do it. */
			data.cb_flag_clear = (id->tag & ID_TAG_NO_USER_REFCOUNT) ? IDWALK_CB_USER : 0;
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
		
		IDP_foreach_property(id->properties, IDP_TYPE_FILTER_ID, KER_lib_query_idpropertiesForeachIDLink_callback, &data);
		if (KER_lib_query_foreachid_iter_stop(&data)) {
			library_foreach_ID_data_cleanup(&data);
			return false;
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
	
#undef CALLBACK_INVOKE_ID
#undef CALLBACK_INVOKE

	library_foreach_ID_data_cleanup(&data);
	return true;
}

void KER_library_foreach_ID_link(Main *main, ID *id, LibraryIDLinkCallback callback, void *user_data, int flag) {
	library_foreach_ID_link(main, NULL, id, callback, user_data, flag, NULL);
}

/** \} */
