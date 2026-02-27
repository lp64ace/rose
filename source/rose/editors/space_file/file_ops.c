#include "DNA_space_types.h"

#include "LIB_string.h"
#include "LIB_fileops.h"
#include "LIB_path_utils.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_main.h"
#include "KER_userdef.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "WM_api.h"
#include "WM_handler.h"

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

		filelist_setdir(sfile->files, params->dir);
	}

	if (filelist_is_dir(sfile->files, params->dir)) {
		/* If directory exists, enter it immediately. */
	}

	ED_fileselect_change_dir(C);
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
	WM_operatortype_append(FILE_OT_execute);
	WM_operatortype_append(FILE_OT_cancel);
}

/** \} */
