#include "DNA_space_types.h"

#include "LIB_string.h"
#include "LIB_fileops.h"
#include "LIB_path_utils.h"
#include "LIB_utildefines.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "KER_context.h"
#include "KER_idprop.h"
#include "KER_main.h"
#include "KER_userdef.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "WM_api.h"
#include "WM_handler.h"

#include "../interface/interface_intern.h"

#include "filelist/filelist.h"
#include "file.h"

/* -------------------------------------------------------------------- */
/** \name File Select Operations
 * \{ */

ROSE_INLINE void file_expand_directory(const Main *main, FileSelectParams *params) {
	/* The path was invalid, fall back to the default root. */
	bool do_reset = false;

	if (params->dir[0] == '\0') {
		do_reset = true;
	}
	else if (LIB_path_is_rel(params->dir)) {
		const char *rosefile_path = KER_main_rosefile_path(main);
		if (rosefile_path[0] != '\0') {
			ROSE_assert_msg(0, "Not implemented yet! TODO!");
		}
		else {
			do_reset = true;
		}
	}
	else if (params->dir[0] == '~') {
		/* While path handling expansion typically doesn't support home directory expansion
		 * in Rose, this is a convenience to be able to type in a single character.
		 * Even though this is a UNIX convention, it's harmless to expand on WIN32 as well. */
		const char *home_dir = NULL;  // LIB_dir_home();
		if (home_dir) {
			char tmpstr[sizeof(params->dir) - 1];
			LIB_strcpy(tmpstr, ARRAY_SIZE(tmpstr), params->dir + 1);
			LIB_path_join(params->dir, sizeof(params->dir), home_dir, tmpstr);
		}
		else {
			do_reset = true;
		}
	}
#ifdef WIN32
	else if (LIB_path_is_win32_drive_only(params->dir)) {
		/* Change `C:` --> `C:\`, see: #28102. */
		params->dir[2] = SEP;
		params->dir[3] = '\0';
	}
	else if (LIB_path_is_unc(params->dir)) {
		LIB_path_normalize_unc(params->dir, ARRAY_SIZE(params->dir));
	}
#endif

	if (do_reset) {
		LIB_path_current_working_directory(params->dir, ARRAY_SIZE(params->dir));
	}
}

void file_directory_enter_handle(struct rContext *C, struct uiBut *but, void *unused1, void *unused2) {
	ScrArea *area = CTX_wm_area(C);
	SpaceFile *sfile = CTX_wm_space_file(C);
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	if (params == NULL) {
		return;
	}

	Main *main = CTX_data_main(C);

	file_expand_directory(main, params);

	/* Special case, user may have pasted a path including a file-name into the directory. */
	if (!filelist_is_dir(sfile->files, params->dir)) {
		if (LIB_is_file(params->dir)) {
			char tmp[FILE_MAX];
			LIB_strcpy(tmp, ARRAY_SIZE(tmp), params->dir);
			LIB_path_split_dir_file(tmp, params->dir, sizeof(params->dir), params->file, sizeof(params->file));

			ED_area_tag_redraw(area);
		}
	}

	if (filelist_is_dir(sfile->files, params->dir)) {
		/* If directory exists, enter it immediately. */
	}

	ED_fileselect_change_dir(C);
	ED_area_tag_redraw(area);
}

void file_draw_check_ex(rContext *C, struct ScrArea *area) {
	SpaceFile *sfile = (SpaceFile *)(area->spacedata.first);
	wmOperator *op = sfile->op;
	if (op) { /* fail on reload */
		if (op->type->check) {
			Main *main = CTX_data_main(C);
			file_sfile_to_operator(C, main, op, sfile);

			/* redraw */
			if (op->type->check(C, op)) {
				file_operator_to_sfile(main, sfile, op);

				/* redraw, else the changed settings won't get updated */
				ED_area_tag_redraw(area);
			}
		}
	}
}

void file_draw_check(rContext *C) {
	ScrArea *area = CTX_wm_area(C);
	file_draw_check_ex(C, area);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Execute File Window Operator
 * \{ */

/**
 * Execute the active file, as set in the file select params.
 */
ROSE_INLINE bool file_execute(rContext *C, SpaceFile *sfile) {
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);

	/* Opening file, sends events now, so things get handled on window-queue level. */
	if (sfile->op) {
		ScrArea *area = CTX_wm_area(C);
		wmOperator *op = sfile->op;

		sfile->op = NULL;

		WM_event_fileselect_event(CTX_wm_manager(C), op, EVT_FILESELECT_EXEC);
	}

	return true;
}

ROSE_INLINE wmOperatorStatus file_exec(rContext *C, wmOperator *op) {
	SpaceFile *sfile = CTX_wm_space_file(C);

	if (!file_execute(C, sfile)) {
		return OPERATOR_CANCELLED;
	}

	return OPERATOR_FINISHED;
}

void FILE_OT_execute(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Execute File Window";
	ot->description = "Execute selected file";
	ot->idname = "FILE_OT_execute";

	/* API callbacks. */
	ot->exec = file_exec;
	/* Important since handler is on window level.
	 *
	 * Avoid using #file_operator_poll since this is also used for entering directories
	 * which is used even when the file manager doesn't have an operator. */
	ot->poll = ED_operator_file_browsing_active;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Cancel File Selector Operator
 * \{ */

ROSE_INLINE bool file_operator_poll(rContext *C) {
	if (ED_operator_file_browsing_active(C)) {
		SpaceFile *sfile = CTX_wm_space_file(C);

		if (sfile && sfile->op) {
			return true;
		}
	}
	return false;
}

ROSE_INLINE wmOperatorStatus file_cancel_exec(rContext *C, wmOperator *unused) {
	WindowManager *wm = CTX_wm_manager(C);
	SpaceFile *sfile = CTX_wm_space_file(C);
	wmOperator *op = sfile->op;

	sfile->op = NULL;

	WM_event_fileselect_event(wm, op, EVT_FILESELECT_CANCEL);

	return OPERATOR_FINISHED;
}

void FILE_OT_cancel(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Cancel File Operation";
	ot->description = "Cancel file operation";
	ot->idname = "FILE_OT_cancel";

	/* API callbacks. */
	ot->exec = file_cancel_exec;
	ot->poll = file_operator_poll;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Navigate Parent Operator
 * \{ */

static wmOperatorStatus file_parent_exec(rContext *C, wmOperator *op) {
	Main *main = CTX_data_main(C);
	SpaceFile *sfile = CTX_wm_space_file(C);
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);

	if (params) {
		if (LIB_path_parent_dir(params->dir, ARRAY_SIZE(params->dir))) {
			LIB_path_absolute(params->dir, ARRAY_SIZE(params->dir), params->dir);
			LIB_path_normalize(params->dir);
			ED_fileselect_change_dir(C);
		}
	}

	return OPERATOR_FINISHED;
}

void FILE_OT_parent(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Parent Directory";
	ot->description = "Move to parent directory";
	ot->idname = "FILE_OT_parent";

	/* API callbacks. */
	ot->exec = file_parent_exec;
	/* File browsing only operator (not asset browsing). */
	ot->poll = ED_operator_file_browsing_active; /* <- important, handler is on window level */
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Navigate Previous Operator
 * \{ */

static wmOperatorStatus file_previous_exec(rContext *C, wmOperator *op) {
	SpaceFile *sfile = CTX_wm_space_file(C);
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);

	if (params) {
		ED_folderlist_pushdir(&sfile->folders_next, params->dir);
		ED_folderlist_popdir(&sfile->folders_prev, params->dir);
		ED_folderlist_pushdir(&sfile->folders_next, params->dir);

		ED_fileselect_change_dir(C);
	}

	return OPERATOR_FINISHED;
}

void FILE_OT_previous(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Previous Folder";
	ot->description = "Move to previous folder";
	ot->idname = "FILE_OT_previous";

	/* API callbacks. */
	ot->exec = file_previous_exec;
	/* File browsing only operator (not asset browsing). */
	ot->poll = ED_operator_file_browsing_active; /* <- important, handler is on window level */
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Navigate Next Operator
 * \{ */

static wmOperatorStatus file_next_exec(rContext *C, wmOperator *ot) {
	SpaceFile *sfile = CTX_wm_space_file(C);
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	if (params) {
		ED_folderlist_pushdir(&sfile->folders_prev, params->dir);
		ED_folderlist_popdir(&sfile->folders_next, params->dir);
		/* update folders_prev so we can check for it in #folderlist_clear_next() */
		ED_folderlist_pushdir(&sfile->folders_prev, params->dir);

		ED_fileselect_change_dir(C);
	}

	return OPERATOR_FINISHED;
}

void FILE_OT_next(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Next Folder";
	ot->description = "Move to next folder";
	ot->idname = "FILE_OT_next";

	/* API callbacks. */
	ot->exec = file_next_exec;
	/* File browsing only operator (not asset browsing). */
	ot->poll = ED_operator_file_browsing_active; /* <- important, handler is on window level */
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Select Operator
 * \{ */

typedef struct FileSelectOperator_CustomData {
	int event;
} FileSelectOperator_CustomData;

ROSE_INLINE wmOperatorStatus file_select_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	ARegion *region = CTX_wm_region(C);

	RNA_int_set(op->ptr, "x", event->mouse_xy[0]);
	RNA_int_set(op->ptr, "y", event->mouse_xy[1]);

	op->customdata = MEM_callocN(sizeof(FileSelectOperator_CustomData), "FileSelectOperator_CustomData");

	return op->type->modal(C, op, event);
}

ROSE_INLINE wmOperatorStatus file_select_modal(rContext *C, wmOperator *op, const wmEvent *event) {
	PropertyRNA *wait_to_deselect_prop = RNA_struct_find_property(op->ptr, "wait_to_deselect_others");

	/* Get settings from RNA properties for operator. */
	const int mouse_begin[2] = {
		RNA_int_get(op->ptr, "x"),
		RNA_int_get(op->ptr, "y"),
	};

	FileSelectOperator_CustomData *data = (FileSelectOperator_CustomData *)op->customdata;

	const bool use_select_on_click = RNA_struct_property_is_set(op->ptr, "use_select_on_click");
	const int init_event_type = data->event;

	if (init_event_type == EVENT_NONE) {
		data->event = event->type;

		if (use_select_on_click) {
			/* Don't do any selection yet. Wait to see if there's a drag or click (release) event. */
			WM_event_add_modal_handler(C, op);
			return OPERATOR_RUNNING_MODAL | OPERATOR_PASS_THROUGH;
		}

		if (event->value == KM_PRESS) {
			RNA_property_boolean_set(op->ptr, wait_to_deselect_prop, true);

			wmOperatorStatus retval = op->type->exec(C, op);

			if (retval & OPERATOR_RUNNING_MODAL) {
				WM_event_add_modal_handler(C, op);
			}

			return retval | OPERATOR_PASS_THROUGH;
		}

		/**
		 * If we are in init phase, and cannot validate init of modal operations,
		 * just fall back to basic exec.
		 */
		RNA_property_boolean_set(op->ptr, wait_to_deselect_prop, false);

		return op->type->exec(C, op);
	}
	if (init_event_type == event->type && event->value == KM_RELEASE) {
		RNA_property_boolean_set(op->ptr, wait_to_deselect_prop, false);

		wmOperatorStatus retval = op->type->exec(C, op);
		return retval | OPERATOR_PASS_THROUGH;
	}

	if (ISMOUSE_MOTION(event->type)) {
		const int drag_delta[2] = {
			mouse_begin[0] - event->mouse_xy[0],
			mouse_begin[1] - event->mouse_xy[1],
		};

		/**
		 * If user moves mouse more than defined threshold, we consider select operator as
		 * finished. Otherwise, it is still running until we get an 'release' event. In any
		 * case, we pass through event, but select op is not finished yet.
		 */
		// if (WM_event_drag_test_with_delta(event, drag_delta)) {
		// 	return OPERATOR_FINISHED | OPERATOR_PASS_THROUGH;
		// }

		/**
		 * Important not to return anything other than PASS_THROUGH here,
		 * otherwise it prevents underlying drag detection code to work properly.
		 */
		return OPERATOR_PASS_THROUGH;
	}

	return OPERATOR_RUNNING_MODAL | OPERATOR_PASS_THROUGH;
}

ROSE_INLINE size_t file_select_find_file(ARegion *region, int xy[2]) {
	View2D *v2d = &region->v2d;

	// This is really bad to do here!
	LISTBASE_FOREACH(uiBlock *, block, &region->uiblocks) {
		if (!block->active) {
			continue;
		}

		float x = xy[0], y = xy[1];
		ui_window_to_block_fl(region, block, &x, &y);

		if (STREQ(block->name, "FILEBROWSER_PT_file_list")) {
			LISTBASE_FOREACH(uiBut *, but, &block->buttons) {
				if (but->pointype != UI_POINTER_NIL) {
					continue;
				}

				if (ui_but_contains_pt(but, x, y)) {
					return POINTER_AS_INT(but->poin);
				}
			}
		}
	}

	return -1;
}

typedef enum eFileSelect {
	FILE_SELECT_NOTHING = 0,
	FILE_SELECT_DIR = 1,
	FILE_SELECT_FILE = 2,
} eFileSelect;

ROSE_INLINE eFileSelect file_select_do(rContext *C, size_t index, bool do_diropen) {
	Main *main = CTX_data_main(C);
	SpaceFile *sfile = CTX_wm_space_file(C);
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);

	size_t total = filelist_files_ensure(sfile->files);
	const FileDirEntry *file;

	eFileSelect ret = FILE_SELECT_NOTHING;
	if ((0 <= index) && (index < total) && (file = filelist_file(sfile->files, index))) {
		if ((file->type & FILE_TYPE_DIR) != 0) {
			const bool is_parent_dir = FILENAME_IS_PARENT(file->relpath);

			if (do_diropen == false) {
				ret = FILE_SELECT_DIR;
			}
			/* the path is too long and we are not going up! */
			else if (!is_parent_dir && LIB_strlen(params->dir) + LIB_strlen(file->relpath) >= FILE_MAX) {
				// Path too long, cannot enter this directory
			}
			else {
				if (is_parent_dir) {
					/* Avoids "/../../" */
					LIB_path_parent_dir(params->dir, ARRAY_SIZE(params->dir));
				}
				else {
					LIB_path_join(params->dir, ARRAY_SIZE(params->dir), params->dir, file->relpath);
					LIB_path_normalize(params->dir);
				}
			}

			ED_fileselect_change_dir(C);
			ret = FILE_SELECT_DIR;
		}
		else {
			ret = FILE_SELECT_FILE;
		}

		filelist_file_set(C, sfile, index);
	}

	return ret;
}

ROSE_INLINE void file_select_deselect_all(SpaceFile *sfile, const int flag) {
	size_t numfiles = filelist_files_ensure(sfile->files);

	for (size_t index = 0; index < numfiles; index++) {
		FileDirEntry *file = filelist_file(sfile->files, index);

		file->flag &= ~flag;
	}
}

ROSE_INLINE wmOperatorStatus file_select_exec(rContext *C, wmOperator *op) {
	WindowManager *wm = CTX_wm_manager(C);
	ARegion *region = CTX_wm_region(C);
	SpaceFile *sfile = CTX_wm_space_file(C);

	const bool clear = RNA_boolean_get(op->ptr, "deselect_all");
	const bool extend = RNA_boolean_get(op->ptr, "extend");
	const bool fill = RNA_boolean_get(op->ptr, "fill");
	const bool do_open = RNA_boolean_get(op->ptr, "open");

	bool wait_to_deselect_others = RNA_boolean_get(op->ptr, "wait_to_deselect_others");

	int mval[2] = {
		RNA_int_get(op->ptr, "x"),
		RNA_int_get(op->ptr, "y"),
	};

	size_t index = file_select_find_file(region, mval);
	if (index == (size_t)-1) {
		return OPERATOR_CANCELLED | OPERATOR_PASS_THROUGH;
	}

	wmOperatorStatus ret = OPERATOR_FINISHED;

	const FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	if (params) {
		size_t numfiles = filelist_files_ensure(sfile->files);

		bool has_selected = false;

		for (size_t index = 0; index < numfiles; index++) {
			FileDirEntry *file = filelist_file(sfile->files, index);

			if ((file->flag & FILE_SEL_SELECTED) != 0) {
				if (wait_to_deselect_others) {
					ret = OPERATOR_RUNNING_MODAL;
				}

				has_selected |= true;
			}
		}

		/* single select, deselect all selected first */
		if (has_selected && !extend) {
			file_select_deselect_all(sfile, FILE_SEL_SELECTED);
		}
	}

	switch (file_select_do(C, index, do_open)) {
		case FILE_SELECT_NOTHING: {
			if (clear) {
				file_select_deselect_all(sfile, FILE_SEL_SELECTED);
			}
		} break;
		case FILE_SELECT_DIR: {
			// Handled by #file_select_do!
		} break;
		case FILE_SELECT_FILE: {
			FileDirEntry *file = filelist_file(sfile->files, index);

			file->flag |= FILE_SEL_SELECTED;

			if (do_open) {
				WM_event_fileselect_event(wm, sfile->op, EVT_FILESELECT_EXEC);
			}
		} break;
	}

	return ret;
}

void FILE_OT_select(wmOperatorType *ot) {
	/* identifiers */
	ot->name = "Select";
	ot->idname = "FILE_OT_select";
	ot->description = "Handle mouse clicks to select and activate items";

	/* API callbacks. */
	ot->invoke = file_select_invoke;
	ot->exec = file_select_exec;
	ot->modal = file_select_modal;
	/* Operator works for file or asset browsing */
	ot->poll = ED_operator_file_browsing_active;

	/* rna */
	RNA_def_int(ot->srna, "x", 0, INT_MIN, INT_MAX, "X", "", INT_MIN, INT_MAX);
	RNA_def_int(ot->srna, "y", 0, INT_MIN, INT_MAX, "Y", "", INT_MIN, INT_MAX);
	RNA_def_boolean(ot->srna, "wait_to_deselect_others", false, "Wait to Deselect Others", "");

	RNA_def_boolean(ot->srna, "extend", false, "Extend", "Extend selection instead of deselecting everything first");
	RNA_def_boolean(ot->srna, "fill", false, "Fill", "Select everything beginning with the last selection");
	RNA_def_boolean(ot->srna, "open", true, "Open", "Open a directory when selecting it");
	RNA_def_boolean(ot->srna, "deselect_all", false, "Deselect On Nothing", "Deselect all when nothing under the cursor");
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Operator Utilities
 * \{ */

void file_sfile_to_operator_ex(rContext *C, Main *main, wmOperator *op, SpaceFile *sfile, char *filepath) {
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	PropertyRNA *prop;
	char dir[FILE_MAX];

	LIB_strcpy(dir, FILE_MAX, params->dir);

	/* XXX, not real length */
	if (params->file[0]) {
		LIB_path_join(filepath, FILE_MAX, params->dir, params->file);
	}
	else {
		LIB_strcpy(filepath, FILE_MAX, dir);
	}

	if ((prop = RNA_struct_find_property(op->ptr, "relative_path"))) {
		if (RNA_property_boolean_get(op->ptr, prop)) {
			LIB_path_rel(filepath, KER_main_rosefile_path(main));
			LIB_path_rel(dir, KER_main_rosefile_path(main));
		}
	}

	char value[FILE_MAX];
	if ((prop = RNA_struct_find_property(op->ptr, "filename"))) {
		RNA_property_string_get(op->ptr, prop, value);
		RNA_property_string_set(op->ptr, prop, params->file);
		if (RNA_property_update_check(prop) && !STREQ(params->file, value)) {
			RNA_property_update(C, op->ptr, prop);
		}
	}
	if ((prop = RNA_struct_find_property(op->ptr, "directory"))) {
		RNA_property_string_get(op->ptr, prop, value);
		RNA_property_string_set(op->ptr, prop, dir);
		if (RNA_property_update_check(prop) && !STREQ(dir, value)) {
			RNA_property_update(C, op->ptr, prop);
		}
	}
	if ((prop = RNA_struct_find_property(op->ptr, "filepath"))) {
		RNA_property_string_get(op->ptr, prop, value);
		RNA_property_string_set(op->ptr, prop, filepath);
		if (RNA_property_update_check(prop) && !STREQ(filepath, value)) {
			RNA_property_update(C, op->ptr, prop);
		}
	}

	/* some ops have multiple files to select */
	/* this is called on operators check() so clear collections first since
	 * they may be already set. */
	{
		// TODO;
	}
}

void file_sfile_to_operator(rContext *C, Main *main, wmOperator *op, SpaceFile *sfile) {
	char filepath_dummy[FILE_MAX];
	file_sfile_to_operator_ex(C, main, op, sfile, filepath_dummy);
}

void file_operator_to_sfile(Main *main, SpaceFile *sfile, wmOperator *op) {
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	PropertyRNA *prop;

	/* If neither of the above are set, split the filepath back */
	if ((prop = RNA_struct_find_property(op->ptr, "filepath"))) {
		char filepath[FILE_MAX];
		RNA_property_string_get(op->ptr, prop, filepath);
		LIB_path_split_dir_file(filepath, params->dir, sizeof(params->dir), params->file, sizeof(params->file));
	}
	else {
		if ((prop = RNA_struct_find_property(op->ptr, "filename"))) {
			RNA_property_string_get(op->ptr, prop, params->file);
		}
		if ((prop = RNA_struct_find_property(op->ptr, "directory"))) {
			RNA_property_string_get(op->ptr, prop, params->dir);
		}
	}
}

void file_sfile_filepath_set(SpaceFile *sfile, const char *filepath) {
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	ROSE_assert(LIB_exists(filepath));

	if (LIB_is_dir(filepath)) {
		LIB_strcpy(params->dir, ARRAY_SIZE(params->dir), filepath);
	}
	else {
		if ((params->flag & FILE_DIRSEL_ONLY) == 0) {
			LIB_path_split_dir_file(filepath, params->dir, sizeof(params->dir), params->file, sizeof(params->file));
		}
		else {
			LIB_path_split_dir_part(filepath, params->dir, sizeof(params->dir));
		}
	}
}

void file_operatortypes() {
	WM_operatortype_append(FILE_OT_select);
	WM_operatortype_append(FILE_OT_execute);
	WM_operatortype_append(FILE_OT_cancel);
	WM_operatortype_append(FILE_OT_parent);
	WM_operatortype_append(FILE_OT_previous);
	WM_operatortype_append(FILE_OT_next);
}

void file_keymap(wmKeyConfig *keyconf) {
	/* File Selecting ------------------------------------------------ */
	wmKeyMap *keymap = WM_keymap_ensure(keyconf, "File Browser", SPACE_FILE, RGN_TYPE_WINDOW);

	/* clang-format off */

	do {
		wmKeyMapItem *item = WM_keymap_add_item(keymap, "FILE_OT_select", &(KeyMapItem_Params){
			.type = LEFTMOUSE,
			.value = KM_PRESS,
			.modifier = KM_NOTHING,
		});

		RNA_boolean_set(item->ptr, "open", false);
	} while(false);

	do {
		wmKeyMapItem *item = WM_keymap_add_item(keymap, "FILE_OT_select", &(KeyMapItem_Params){
			.type = LEFTMOUSE,
			.value = KM_DBL_CLICK,
			.modifier = KM_NOTHING,
		});

		RNA_boolean_set(item->ptr, "open", true);
	} while(false);

	/* clang-format on */
}

/** \} */
