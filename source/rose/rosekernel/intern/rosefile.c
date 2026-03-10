#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_idtype.h"
#include "KER_layer.h"
#include "KER_library.h"
#include "KER_lib_id.h"
#include "KER_lib_query.h"
#include "KER_lib_remap.h"
#include "KER_main.h"
#include "KER_main_name_map.h"
#include "KER_main_id_name_map.h"
#include "KER_scene.h"
#include "KER_rose.h"
#include "KER_rosefile.h"
#include "KER_userdef.h"

#include "DNA_userdef_types.h"
#include "DNA_windowmanager_types.h"

/* -------------------------------------------------------------------- */
/** \name App Data
 * \{ */

/**
 * Helper struct to manage IDs that are re-used across rose-file loading (i.e. moved from the old
 * Main the new one).
 *
 * NOTE: this is only used when actually loading a real `.rose` file,
 * loading of memfile undo steps does not need it.
 */
typedef struct ReuseOldMainData {
	struct Main *new_main;
	struct Main *old_main;

	/**
	 * Storage for all remapping rules (old_id -> new_id) required by the preservation of old IDs
	 * into the new Main.
	 */
	IDRemapper *remapper;
	bool is_libraries_remapped;

	/**
	 * Used to find matching IDs by name/lib in new main, to remap ID usages of data ported over
	 * from old main.
	 */
	IDNameLib_Map *id_map;
} ReuseOldMainData;

/**
 * Search for all libraries in `old_main` that are also in `new_main` (i.e. different Library
 * IDs having the same absolute filepath), and create a remapping rule for these.
 *
 * NOTE: The case where the `old_main` would be a library in the newly read one is not handled
 * here, as it does not create explicit issues. The local data from `old_main` is either
 * discarded, or added to the `new_main` as local data as well. Worst case, there will be a
 * double of a linked data as a local one, without any known relationships between them. In
 * practice, this latter case is not expected to commonly happen.
 */
ROSE_INLINE IDRemapper *reuse_main_data_remapper_ensure(ReuseOldMainData *data) {
	if (data->is_libraries_remapped) {
		return data->remapper;
	}

	if (data->remapper == NULL) {
		data->remapper = KER_id_remapper_new();
	}

	Main *new_main = data->new_main;
	Main *old_main = data->old_main;
	IDRemapper *remapper = data->remapper;

	LISTBASE_FOREACH(Library *, old_lib_iter, &old_main->libraries) {
		/**
		 * In case newly opened `new_main` is a library of the `old_main`, remap it to NULL, since a
		 * file should never ever have linked data from itself.
		 */
		if (STREQ(old_lib_iter->filepath, new_main->filepath)) {
			KER_id_remapper_add(remapper, &old_lib_iter->id, NULL);
			continue;
		}

		/**
		 * NOTE: Although this is quadratic complexity, it is not expected to be an issue in practice:
		 *  - Files using more than a few tens of libraries are extremely rare.
		 *  - This code is only executed once for every file reading (not on undos).
		 */
		LISTBASE_FOREACH(Library *, new_lib_iter, &new_main->libraries) {
			if (STREQ(old_lib_iter->filepath, new_lib_iter->filepath)) {
				KER_id_remapper_add(remapper, &new_lib_iter->id, NULL);
				break;
			}
		}
	}

	data->is_libraries_remapped = true;
	return remapper;
}

ROSE_STATIC bool reuse_main_data_remapper_is_id_remapped(IDRemapper *remapper, ID *id) {
	int result = KER_id_remapper_mapping_result(remapper, id, ID_REMAP_APPLY_DEFAULT, NULL);
	if (ELEM(result, ID_REMAP_RESULT_SOURCE_REMAPPED, ID_REMAP_RESULT_SOURCE_UNASSIGNED)) {
		/* ID is already remapped to its matching ID in the new main, or explicitly remapped to null,
		 * nothing else to do here. */
		return true;
	}
	ROSE_assert_msg(result != ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE, "There should never be a non-mappable (i.e. null) input here.");
	ROSE_assert(result == ID_REMAP_RESULT_SOURCE_UNAVAILABLE);
	return false;
}

ROSE_STATIC bool reuse_main_move_id(ReuseOldMainData *data, ID *id, Library *lib, const bool reuse_existing) {
	IDRemapper *remapper = reuse_main_data_remapper_ensure(data);

	Main *new_main = data->new_main;
	Main *old_main = data->old_main;
	ListBase *new_lb = which_libbase(new_main, GS(id->name));
	ListBase *old_lb = which_libbase(old_main, GS(id->name));

	if (reuse_existing) {
		/**
		 * A 'new' version of the same data may already exist in new_main, in the rare case
		 * that the same asset rose file was linked explicitly into the rose file we are loading.
		 * Don't move the old linked ID, but remap its usages to the new one instead.
		 */
		LISTBASE_FOREACH_BACKWARD(ID *, id_iter, new_lb) {
			if (!ELEM(id_iter->lib, id->lib, lib)) {
				continue;
			}
			if (!STREQ(id_iter->name + 2, id->name + 2)) {
				continue;
			}

			KER_id_remapper_add(remapper, id, id_iter);
		}
	}

	/** If ID is already in the new_main, this should not have been called. */
	ROSE_assert(!LIB_haslink(new_lb, id));
	ROSE_assert(LIB_haslink(old_lb, id));

	/* Move from one list to another, and ensure name is valid. */
	if (LIB_haslink(old_lb, id)) {
		LIB_remlink(old_lb, id);
	}

	/**
	 * In case the ID is linked and its library ID is re-used from the old Main, it is not possible
	 * to handle name_map (and ensure name uniqueness).
	 * This is because IDs are moved one by one from old Main's lists to new ones, while the re-used
	 * library's name_map would be built only from IDs in the new list, leading to incomplete/invalid
	 * states.
	 * Currently, such name uniqueness checks should not be needed, as no new name would be expected
	 * in the re-used library. Should this prove to be wrong at some point, the name check will have
	 * to happen at the end of #reuse_editable_asset_main_data_for_rosefile, in a separate loop
	 * over Main IDs.
	 */
	const bool handle_name_map_updates = !ID_IS_LINKED(id) || id->lib != lib;
	if (handle_name_map_updates) {
		KER_main_idmap_remove_id(old_main->id_map, id);
	}

	id->lib = lib;
	LIB_addtail(new_lb, id);
	if (handle_name_map_updates) {
		KER_id_new_name_validate(new_main, new_lb, id, NULL);
	}
	else {
		id_sort_by_name(new_lb, id, NULL);
	}
	KER_lib_libblock_session_uuid_renew(id);

	/* Remap to itself, to avoid re-processing this ID again. */
	KER_id_remapper_add(remapper, id, id);
	return true;
}

ROSE_STATIC Library *reuse_main_data_dependencies_new_library_get(ReuseOldMainData *reuse_data, Library *old_lib) {
	const IDRemapper *remapper = reuse_main_data_remapper_ensure(reuse_data);
	Library *new_lib = old_lib;
	int result = KER_id_remapper_mapping_apply(remapper, (void **)&new_lib, ID_REMAP_APPLY_DEFAULT, NULL);

	switch (result) {
		case ID_REMAP_RESULT_SOURCE_UNAVAILABLE: {
			/**
			 * Move library to new main.
			 * There should be no filepath conflicts, as #reuse_main_data_remapper_ensure has
			 * already remapped existing libraries with matching filepath.
			 */
			reuse_main_move_id(reuse_data, &old_lib->id, NULL, false);
			/**
			 * Clear the name_map of the library, as not all of its IDs are guaranteed reused. The name
			 * map cannot be used/kept in valid state while some IDs are moved from old to new main. See
			 * also #reuse_main_move_id code.
			 */
			KER_main_namemap_destroy(&old_lib->runtime.name_map);
		} return old_lib;
		case ID_REMAP_RESULT_SOURCE_NOT_MAPPABLE: {
			ROSE_assert_unreachable();
		} return NULL;
		case ID_REMAP_RESULT_SOURCE_REMAPPED: {
			/* Already in new main */
		} return new_lib;
		case ID_REMAP_RESULT_SOURCE_UNASSIGNED: {
			/* Happens when the library is the newly opened rose file. */
		} return NULL;
	}

	ROSE_assert_unreachable();
	return NULL;
}

ROSE_INLINE int reuse_editable_asset_main_data_dependencies_process_cb(LibraryIDLinkCallbackData *data) {
	ID *id = *data->self_ptr;

	if (id == NULL) {
		return IDWALK_RET_NOP;
	}

	if (GS(id->name) == ID_LI) {
		/* Libraries are handled separately. */
		return IDWALK_RET_STOP_RECURSION;
	}

	ReuseOldMainData *reuse_data = (ReuseOldMainData *)(data->user_data);

	/* First check if it has already been remapped. */
	IDRemapper *remapper = reuse_main_data_remapper_ensure(reuse_data);
	if (reuse_main_data_remapper_is_id_remapped(remapper, id)) {
		return IDWALK_RET_STOP_RECURSION;
	}

	if (id->lib == NULL) {
		/* There should be no links to local datablocks from linked editable data. */
		KER_id_remapper_add(remapper, id, NULL);
		ROSE_assert_unreachable();
		return IDWALK_RET_STOP_RECURSION;
	}

	/* Only preserve specific datablock types. */
	if (!ID_TYPE_SUPPORTS_ASSET_EDITABLE(GS(id->name))) {
		KER_id_remapper_add(remapper, id, NULL);
		return IDWALK_RET_STOP_RECURSION;
	}

	/**
	 * There may be a new library pointer in new_main, matching a library in old_main, even
	 * though pointer values are not the same. So we need to check new linked IDs in new_main
	 * against both potential library pointers.
	 */
	Library *old_id_new_lib = reuse_main_data_dependencies_new_library_get(reuse_data, id->lib);

	/* Happens when the library is the newly opened rose file. */
	if (old_id_new_lib == NULL) {
		KER_id_remapper_add(remapper, id, NULL);
		return IDWALK_RET_STOP_RECURSION;
	}

	/* Move to new main database. */
	if (reuse_main_move_id(reuse_data, id, old_id_new_lib, true)) {
		return IDWALK_RET_STOP_RECURSION;
	}

	return IDWALK_RET_NOP;
}

ROSE_INLINE void reuse_editable_asset_main_data_for_rosefile(ReuseOldMainData *reuse_data, const short idcode) {
	Main *new_main = reuse_data->new_main;
	Main *old_main = reuse_data->old_main;

	IDRemapper *remapper = reuse_main_data_remapper_ensure(reuse_data);

	ListBase *old_lb = which_libbase(old_main, idcode);

	ID *old_id_iter;
	FOREACH_MAIN_LISTBASE_ID_BEGIN(old_lb, old_id_iter) {
		/* Keep any datablocks from libraries marked as LIBRARY_ASSET_EDITABLE. */
		if (!ID_IS_LINKED(old_id_iter)) {
			continue;
		}

		Library *old_id_new_lib = reuse_main_data_dependencies_new_library_get(reuse_data, old_id_iter->lib);

		/* Happens when the library is the newly opened rose file. */
		if (old_id_new_lib == NULL) {
			KER_id_remapper_add(remapper, old_id_iter, NULL);
			continue;
		}

		if (reuse_main_move_id(reuse_data, old_id_iter, old_id_new_lib, true)) {
			/**
			 * Port over dependencies of re-used ID, unless matching already existing ones in
			 * new_main can be found.
			 *
			 * NOTE : No pointers are remapped here, this code only moves dependencies from old_main
			 * to new_main if needed, and add necessary remapping rules to the reuse_data.remapper.
			 */
			KER_library_foreach_ID_link(new_main, old_id_iter, reuse_editable_asset_main_data_dependencies_process_cb, reuse_data, IDWALK_RECURSE | IDWALK_DO_LIBRARY_POINTER);
		}
	}
	FOREACH_MAIN_LISTBASE_ID_END;
}

/**
 * Does a complete replacement of data in `new_main` by data from `old_main`. Original new data
 * are moved to the `old_main`, and will be freed together with it.
 *
 * WARNING: Currently only expects to work on local data, won't work properly if some of the IDs of
 * given type are linked.
 *
 * NOTE: Unlike with #reuse_editable_asset_main_data_for_rosefile, there is no support at all for
 * potential dependencies of the IDs moved around. This is not expected to be necessary for the
 * current use cases (UI-related IDs).
 */
ROSE_INLINE void swap_old_main_data_for_rosefile(ReuseOldMainData *reuse_data, const short id_code) {
	Main *new_main = reuse_data->new_main;
	Main *old_main = reuse_data->old_main;

	ListBase *new_lb = which_libbase(new_main, id_code);
	ListBase *old_lb = which_libbase(old_main, id_code);

	IDRemapper *remapper = reuse_main_data_remapper_ensure(reuse_data);

	/* NOTE: Full swapping is only supported for ID types that are assumed to be only local
	 * data-blocks (like UI-like ones). Otherwise, the swapping could fail in many funny ways. */
	ROSE_assert(LIB_listbase_is_empty(old_lb) || !ID_IS_LINKED((ID *)old_lb->last));
	ROSE_assert(LIB_listbase_is_empty(new_lb) || !ID_IS_LINKED((ID *)new_lb->last));

	SWAP(ListBase, *new_lb, *old_lb);

	KER_main_idmap_clear(old_main->id_map);
	KER_main_idmap_clear(new_main->id_map);

	/**
	 * Original 'new' IDs have been moved into the old listbase and will be discarded (deleted).
	 * Original 'old' IDs have been moved into the new listbase and are being reused (kept).
	 * The discarded ones need to be remapped to a matching reused one, based on their names, if
	 * possible.
	 *
	 * Since both lists are ordered, and they are all local, we can do a smart parallel processing of
	 * both lists here instead of doing complete full list searches.
	 */
	ID *discarded_id_iter = (ID *)(old_lb->first);
	ID *reused_id_iter = (ID *)(new_lb->first);
	while (!ELEM(NULL, discarded_id_iter, reused_id_iter)) {
		const int strcmp_result = strcmp(discarded_id_iter->name + 2, reused_id_iter->name + 2);
		if (strcmp_result == 0) {
			/* Matching IDs, we can remap the discarded 'new' one to the re-used 'old' one. */
			KER_id_remapper_add(remapper, discarded_id_iter, reused_id_iter);

			discarded_id_iter = (ID *)(discarded_id_iter->next);
			reused_id_iter = (ID *)(reused_id_iter->next);
		}
		else if (strcmp_result < 0) {
			/* No matching reused 'old' ID for this discarded 'new' one. */
			KER_id_remapper_add(remapper, discarded_id_iter, NULL);

			discarded_id_iter = (ID *)(discarded_id_iter->next);
		}
		else {
			reused_id_iter = (ID *)(reused_id_iter->next);
		}
	}
	/* Also remap all remaining non-compared discarded 'new' IDs to null. */
	for (; discarded_id_iter != NULL; discarded_id_iter = (ID *)(discarded_id_iter->next)) {
		KER_id_remapper_add(remapper, discarded_id_iter, NULL);
	}

	FOREACH_MAIN_LISTBASE_ID_BEGIN(new_lb, reused_id_iter) {
		/* Necessary as all `session_uid` are renewed on rosefile loading. */
		KER_lib_libblock_session_uuid_renew(reused_id_iter);
		/* Ensure that the reused ID is remapped to itself, since it is known to be in the `new_main`. */
		KER_id_remapper_add(remapper, reused_id_iter, reused_id_iter);
	}
	FOREACH_MAIN_LISTBASE_ID_END;
}

ROSE_INLINE void swap_wm_data_for_rosefile(ReuseOldMainData *reuse_data, const bool load_ui) {
	Main *new_main = reuse_data->new_main;
	Main *old_main = reuse_data->old_main;
	ListBase *old_wm_list = &old_main->wm;
	ListBase *new_wm_list = &new_main->wm;

	/* Currently there should never be more than one WM in a main. */
	ROSE_assert(LIB_listbase_is_empty(old_wm_list) || LIB_listbase_is_single(old_wm_list));
	ROSE_assert(LIB_listbase_is_empty(new_wm_list) || LIB_listbase_is_single(new_wm_list));

	WindowManager *old_wm = (WindowManager *)old_wm_list->first;
	WindowManager *new_wm = (WindowManager *)new_wm_list->first;

	if (old_wm == NULL) {
		return;
	}

	if (load_ui && new_wm != NULL) {
		LIB_remlink(old_wm_list, old_wm);
		LIB_remlink(new_wm_list, new_wm);
		
		/* WindowManager and UI swapping is not currently implemented! */
		ROSE_assert_unreachable();
	}
}

ROSE_INLINE int swap_old_main_data_for_rosefile_dependencies_process_cb(LibraryIDLinkCallbackData *cb_data) {
	ID *id = *cb_data->self_ptr;

	if (id == NULL) {
		return IDWALK_RET_NOP;
	}

	ReuseOldMainData *reuse_data = (ReuseOldMainData *)cb_data->user_data;

	IDRemapper *remapper = reuse_main_data_remapper_ensure(reuse_data);
	if (reuse_main_data_remapper_is_id_remapped(remapper, id)) {
		return IDWALK_RET_NOP;
	}

	IDNameLib_Map *id_map = reuse_data->id_map;
	ROSE_assert(id_map != NULL);

	ID *id_new = KER_main_idmap_lookup_name(id_map, GS(id), KER_id_name(id));
	KER_id_remapper_add(remapper, id, id_new);
	return IDWALK_RET_NOP;
}

ROSE_STATIC void swap_old_main_data_dependencies_process(ReuseOldMainData *reuse_data, const short idcode) {
	Main *new_main = reuse_data->new_main;
	ListBase *new_lb = which_libbase(new_main, idcode);

	ROSE_assert(reuse_data->id_map != NULL);

	ID *new_id_iter;
	FOREACH_MAIN_LISTBASE_ID_BEGIN(new_lb, new_id_iter) {
		/* Check all ID usages and find a matching new ID to remap them to in `new_bmain` if possible
		 * (matching by names and libraries).
		 *
		 * Note that this call does not do any effective remapping, it only adds required remapping
		 * operations to the remapper. */
		KER_library_foreach_ID_link(new_main, new_id_iter, swap_old_main_data_for_rosefile_dependencies_process_cb, reuse_data, IDWALK_READONLY | IDWALK_INCLUDE_UI | IDWALK_DO_LIBRARY_POINTER);
	}
	FOREACH_MAIN_LISTBASE_ID_END;
}

enum {
	LOAD_UI = 1,
	LOAD_UI_OFF,
	LOAD_UNDO,
};

ROSE_STATIC void setup_app_data(rContext *C, RoseFileData *rfd) {
	Main *main = G_MAIN;

	int mode;

	if (ELEM(NULL, rfd->scene, rfd->screen)) {
		mode = LOAD_UI_OFF;
	}
	else {
		mode = LOAD_UI;
	}

	ReuseOldMainData reuse_data = {
		.new_main = rfd->main,
		.old_main = main,

		.remapper = NULL,
		.is_libraries_remapped = false,

		.id_map = NULL,
	};

	if (mode != LOAD_UNDO) {
		const short ui_id_codes[] = {ID_SCR};

		/* WM needs special complex handling, regardless of whether UI is kept or loaded from file. */
		swap_wm_data_for_rosefile(&reuse_data, mode == LOAD_UI);
		if (mode != LOAD_UI) {
			for (size_t index = 0; index < ARRAY_SIZE(ui_id_codes); index++) {
				swap_old_main_data_for_rosefile(&reuse_data, ui_id_codes[index]);
			}
		}

		/* Needs to happen after all data from `old_main` has been moved into new one. */
		ROSE_assert(reuse_data.id_map == NULL);
		reuse_data.id_map = KER_main_idmap_create(reuse_data.new_main, true, reuse_data.old_main);

		swap_old_main_data_dependencies_process(&reuse_data, ID_WM);
		if (mode != LOAD_UI) {
			for (size_t index = 0; index < ARRAY_SIZE(ui_id_codes); index++) {
				swap_old_main_data_dependencies_process(&reuse_data, ui_id_codes[index]);
			}
		}

		KER_main_idmap_free(reuse_data.id_map);

		for (short idtype_index = 0; idtype_index < INDEX_ID_MAX; idtype_index++) {
			const IDTypeInfo *idtype_info = KER_idtype_get_info_from_idtype_index(idtype_index);
			if (ID_TYPE_SUPPORTS_ASSET_EDITABLE(idtype_info->idcode)) {
				reuse_editable_asset_main_data_for_rosefile(&reuse_data, idtype_info->idcode);
			}
		}
	}

	/* Always use the Scene and ViewLayer pointers from new file, if possible. */
	Scene *scene = rfd->scene;
	ViewLayer *view_layer = rfd->view_layer;

	/* Ensure that there is a valid scene and view-layer. */
	if (scene == NULL) {
		scene = (Scene *)(rfd->main->scenes.first);
	}
	if (scene == NULL) {
		scene = KER_scene_new(rfd->main, NULL);
	}
	if (view_layer == NULL) {
		/* Fallback to the active scene view layer. */
		view_layer = KER_view_layer_default_view(scene);
	}

	wmWindow *window = NULL;
	Screen *screen = NULL;

	if (mode != LOAD_UI) {
		window = CTX_wm_window(C);
		screen = CTX_wm_screen(C);

		if (window) {
			window->scene = scene;
		}
	}

	/**
	 * Apply remapping of ID pointers caused by re-using part of the data from the 'old' main into
	 * the new one.
	 */
	if (reuse_data.remapper != NULL) {
		/* Handle all pending remapping from swapping old and new IDs around. */
		KER_libblock_remap_multiple_raw(rfd->main, reuse_data.remapper, ID_REMAP_FORCE_UI_POINTERS | ID_REMAP_SKIP_USER_REFCOUNT | ID_REMAP_SKIP_USER_CLEAR);

		KER_id_remapper_free(reuse_data.remapper);
		reuse_data.remapper = NULL;
	}

	if (mode != LOAD_UI) {
		if (window) {
			scene = window->scene;
		}
	}

	CTX_data_scene_set(C, scene);

	/* This frees the `old_main`. */
	KER_rose_globals_main_replace(rfd->main);
	main = G_MAIN;
	rfd->main = NULL;

	CTX_data_main_set(C, main);

	if (mode == LOAD_UI) {
		CTX_wm_manager_set(C, (WindowManager *)(main->wm.first));
		CTX_wm_screen_set(C, rfd->screen);
		CTX_wm_area_set(C, NULL);
		CTX_wm_region_set(C, NULL);
	}
	ROSE_assert(CTX_wm_manager(C) == (WindowManager *)main->wm.first);

	/**
	 * Both undo and regular file loading can perform some fairly complex ID manipulation, simpler
	 * and safer to fully redo reference-counting. This is a relatively cheap process anyway.
	 */
	KER_main_id_refcount_recompute(main, false);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

ROSE_STATIC void setup_app_userdef(RoseFileData *rfd) {
	if (rfd->user) {
		KER_rose_userdef_data_set_and_free(rfd->user);
		rfd->user = NULL;
	}
}

ROSE_STATIC void setup_app_main(RoseFileData *rfd) {
	if (rfd->main) {
		// Handle new main data-block(s) here!
		rfd->main = NULL;
	}
}

ROSE_STATIC void setup_app_rose_file_data(RoseFileData *rfd) {
	setup_app_userdef(rfd);
	setup_app_main(rfd);
}

void KER_rosefile_read_setup(RoseFileData *rfd) {
	setup_app_rose_file_data(rfd);
}

/** \} */
