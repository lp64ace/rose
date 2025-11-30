#ifndef KER_LIB_QUERY_H
#define KER_LIB_QUERY_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct LibraryIDLinkCallbackData;
struct LibraryForeachIDData;
struct Main;

/* -------------------------------------------------------------------- */
/** \name Data structures
 * \{ */

typedef struct LibraryIDLinkCallbackData {
	void *user_data;

	/** Main database used to call `KER_library_foreach_ID_link()`. */
	struct Main *main;
	struct ID *owner_id;
	struct ID *self_id;
	struct ID **self_ptr;
	int cb_flag;
} LibraryIDLinkCallbackData;

/** #LibraryIDLinkCallbackData->cb_flag */
enum {
	IDWALK_CB_NOP = 0,
	IDWALK_CB_NEVER_NULL = (1 << 0),
	IDWALK_CB_NEVER_SELF = (1 << 1),

	/**
	 * Indicates that this is an internal runtime ID pointer.
	 * \note Those should be ignored in most cases, and won't be processed/generated anyway unless
	 * `IDWALK_DO_INTERNAL_RUNTIME_POINTERS` option is enabled.
	 */
	IDWALK_CB_INTERNAL = (1 << 2),
	/**
	 * This ID usage is fully refcounted.
	 * Callback is responsible to deal accordingly with #ID.user if needed.
	 */
	IDWALK_CB_USER = (1 << 3),
};

typedef int (*LibraryIDLinkCallback)(LibraryIDLinkCallbackData *);

/** #LibraryIDLinkCallback */
enum {
	IDWALK_RET_NOP = 0,
	/**
	 * Completely stop iteration.
	 *
	 * \note Should never be returned by a callback in case #IDWALK_READONLY is not set.
	 */
	IDWALK_RET_STOP_ITER = 1 << 0,
	/**
	 * Stop recursion, that is, do not loop over ID used by current one.
	 *
	 * \note Should never be returned by a callback in case #IDWALK_READONLY is not set.
	 */
	IDWALK_RET_STOP_RECURSION = 1 << 1,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Foreach Methods
 * \{ */

/**
 * Check whether current iteration over ID usages should be stopped or not.
 * \return true if the iteration should be stopped, false otherwise.
 */
bool KER_lib_query_foreachid_iter_stop(const struct LibraryForeachIDData *data);

void KER_lib_query_foreachid_process(struct LibraryForeachIDData *data, struct ID **id_pp, int cb_flag);

int KER_lib_query_foreachid_process_flags_get(const struct LibraryForeachIDData *data);
struct Main *KER_lib_query_foreachid_process_main_get(const struct LibraryForeachIDData *data);
int KER_lib_query_foreachid_process_callback_flag_override(struct LibraryForeachIDData *data, int cb_flag, bool do_replace);

/** Should typically only be used when processing deprecated ID types (like IPO ones). */
#define KER_LIB_FOREACHID_PROCESS_ID_NOCHECK(data_, id_, cb_flag_)           \
	{                                                                        \
		KER_lib_query_foreachid_process((data_), (ID **)&(id_), (cb_flag_)); \
		if (KER_lib_query_foreachid_iter_stop((data_))) {                    \
			return;                                                          \
		}                                                                    \
	}                                                                        \
	((void)0)

#define KER_LIB_FOREACHID_PROCESS_ID(data_, id_, cb_flag_)          \
	{                                                               \
		KER_LIB_FOREACHID_PROCESS_ID_NOCHECK(data_, id_, cb_flag_); \
	}                                                               \
	((void)0)

#define KER_LIB_FOREACHID_PROCESS_IDSUPER_P(data_, id_super_p_, cb_flag_)           \
	{                                                                               \
		KER_lib_query_foreachid_process((data_), (ID **)(id_super_p_), (cb_flag_)); \
		if (KER_lib_query_foreachid_iter_stop((data_))) {                           \
			return;                                                                 \
		}                                                                           \
	}                                                                               \
	((void)0)

#define KER_LIB_FOREACHID_PROCESS_IDSUPER(data_, id_super_, cb_flag_)       \
	{                                                                       \
		KER_LIB_FOREACHID_PROCESS_IDSUPER_P(data_, &(id_super_), cb_flag_); \
	}                                                                       \
	((void)0)

#define KER_LIB_FOREACHID_PROCESS_FUNCTION_CALL(data_, func_call_) \
	{                                                              \
		func_call_;                                                \
		if (KER_lib_query_foreachid_iter_stop((data_))) {          \
			return;                                                \
		}                                                          \
	}                                                              \
	((void)0)

void KER_lib_query_idpropertiesForeachIDLink_callback(struct IDProperty *prop, void *data);

/* Flags for the foreach function itself. */
enum {
	IDWALK_NOP = 0,
	/**
	 * The callback will never modify the ID pointers it processes.
	 * WARNING: It is very important to pass this flag when valid, as it can lead to important
	 * optimizations and debug/assert code.
	 */
	IDWALK_READONLY = (1 << 0),
	/**
	 * Recurse into 'descendant' IDs.
	 * Each ID is only processed once. Order of ID processing is not guaranteed.
	 *
	 * Also implies #IDWALK_READONLY, and excludes #IDWALK_DO_INTERNAL_RUNTIME_POINTERS.
	 *
	 * NOTE: When enabled, embedded IDs are processed separately from their owner, as if they were
	 * regular IDs. The owner ID remains available in the #LibraryForeachIDData callback data, unless
	 * #IDWALK_IGNORE_MISSING_OWNER_ID is passed.
	 */
	IDWALK_RECURSE = (1 << 1),
	/** Include UI pointers (from WM and screens editors). */
	IDWALK_INCLUDE_UI = (1 << 2),

	/**
	 * Also process internal ID pointers like `ID.newid` or `ID.orig_id`.
	 * WARNING: Dangerous, use with caution.
	 */
	IDWALK_DO_INTERNAL_RUNTIME_POINTERS = (1 << 3),
	/**
	 * Also process the ID.lib pointer. It is an option because this pointer can usually be fully
	 * ignored.
	 */
	IDWALK_DO_LIBRARY_POINTER = (1 << 4),
};

/**
 * Loop over all of the ID's this data-block links to.
 *
 * \param main The Main data-base containing `owner_id`, may be NULL.
 * \param id The ID to process. Note that currently, embedded IDs may also be passed here.
 * \param callback The callback processing a given ID usage (i.e. a given ID pointer within the given \a id data).
 * \param user_data Opaque user data for the callback processing a given ID usage.
 * \param flag Flags controlling how/which ID pointers are processed.
 */
void KER_library_foreach_ID_link(struct Main *main, struct ID *id, LibraryIDLinkCallback callback, void *user_data, int flag);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_LIB_QUERY_H
