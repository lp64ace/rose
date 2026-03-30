#include "MEM_guardedalloc.h"

#include "KER_context.h"
#include "KER_main.h"
#include "KER_rosefile.h"
#include "KER_report.h"

#include "RNA_access.h"

#include "LIB_string.h"
#include "LIB_path_utils.h"
#include "LIB_vector.hh"

#include "IO_fbx.h"

#include "WM_api.h"
#include "WM_handler.h"

#include "RLO_writefile.h"

ROSE_INLINE void wm_rose_recent_filepath_set(rContext *C, wmOperator *op) {
	Main *main = CTX_data_main(C);

	PropertyRNA *property = RNA_struct_find_property(op->ptr, "filepath");
	if (!RNA_property_is_set(op->ptr, property)) {
		const char *rosefile = KER_main_rosefile_path(main);

		char filepath[FILE_MAX];
		if (!rosefile[0] == '\0') {
			LIB_strcpy(filepath, ARRAY_SIZE(filepath), rosefile);
			RNA_property_string_set(op->ptr, property, filepath);
		}
	}
}

/* -------------------------------------------------------------------- */
/** \name Rose Open
 * \{ */

ROSE_STATIC wmOperatorStatus wm_rose_open_select_file_path_exec(rContext *C, wmOperator *op, const wmEvent *event) {
	Main *main = CTX_data_main(C);
	const char *rosefile_path = KER_main_rosefile_path(main);

	if (CTX_wm_window(C) == nullptr) {
		return OPERATOR_CANCELLED;
	}

	wm_rose_recent_filepath_set(C, op);

	WM_event_add_fileselect(C, op);
	return OPERATOR_RUNNING_MODAL;
}

ROSE_STATIC wmOperatorStatus wm_rose_open_exec(rContext *C, wmOperator *op) {
	const bool is_filepath_set = RNA_struct_property_is_set(op->ptr, "filepath");
	if (!is_filepath_set) {
		return OPERATOR_CANCELLED;
	}

	char filepath[FILE_MAX];
	RNA_string_get(op->ptr, "filepath", filepath);

	if (filepath[0]) {
		RoseFileData *rfd = KER_rosefile_read(filepath, 0);
		if (!rfd) {
			KER_reportf(op->reports, RPT_ERROR, "[WM] Cannot load file \"%s\"", filepath);
			return OPERATOR_CANCELLED;
		}

		KER_rosefile_read_setup(C, rfd);

		KER_reportf(op->reports, RPT_INFO, "[WM] Loaded \"%s\"", filepath);
	}

	return OPERATOR_FINISHED;
}

ROSE_STATIC bool wm_rose_open_check(rContext *C, wmOperator *op) {
	return false;
}

ROSE_STATIC bool wm_rose_open_poll(rContext *C) {
	if (!CTX_wm_window(C)) {
		return false;
	}
	return true;
}

void WM_OT_open_mainfile(wmOperatorType *ot) {
	ot->name = "Open";
	ot->description = "Open rose file from disk";
	ot->idname = "WM_OT_open_mainfile";

	ot->invoke = wm_rose_open_select_file_path_exec;
	ot->exec = wm_rose_open_exec;
	ot->check = wm_rose_open_check;
	ot->poll = wm_rose_open_poll;

	WM_operator_properties_filesel(ot, FILE_TYPE_FOLDER, FILE_ROSE, FILE_OPENFILE, WM_FILESEL_FILEPATH | WM_FILESEL_DIRECTORY | WM_FILESEL_FILES);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Rose Save
 * \{ */

ROSE_STATIC wmOperatorStatus wm_rose_save_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	wm_rose_recent_filepath_set(C, op);
	
	WM_event_add_fileselect(C, op);
	return OPERATOR_RUNNING_MODAL;
}

ROSE_STATIC wmOperatorStatus wm_rose_save_exec(rContext *C, wmOperator *op) {
	const bool is_filepath_set = RNA_struct_property_is_set(op->ptr, "filepath");
	if (!is_filepath_set) {
		return OPERATOR_CANCELLED;
	}

	char filepath[FILE_MAX];
	RNA_string_get(op->ptr, "filepath", filepath);

	Main *main = CTX_data_main(C);

	if (filepath[0]) {
		RLO_write_file(main, filepath, 0);
	}

	return OPERATOR_FINISHED;
}

ROSE_STATIC bool wm_rose_save_check(rContext *C, wmOperator *op) {
	return false;
}

ROSE_STATIC bool wm_rose_save_poll(rContext *C) {
	if (!CTX_wm_window(C)) {
		return false;
	}
	return true;
}

void WM_OT_save_mainfile(wmOperatorType *ot) {
	ot->name = "Save";
	ot->description = "Save rose file to disk";
	ot->idname = "WM_OT_save_mainfile";

	ot->invoke = wm_rose_save_invoke;
	ot->exec = wm_rose_save_exec;
	ot->check = wm_rose_save_check;
	ot->poll = wm_rose_save_poll;

	WM_operator_properties_filesel(ot, FILE_TYPE_FOLDER, FILE_ROSE, FILE_SAVE, WM_FILESEL_FILEPATH);
}

/** \} */
