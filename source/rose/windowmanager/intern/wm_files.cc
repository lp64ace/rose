#include "MEM_guardedalloc.h"

#include "KER_context.h"
#include "KER_main.h"
#include "KER_rosefile.h"

#include "RNA_access.h"

#include "LIB_path_utils.h"
#include "LIB_vector.hh"

#include "IO_fbx.h"

#include "WM_api.h"
#include "WM_handler.h"

#include "RLO_writefile.h"

/* -------------------------------------------------------------------- */
/** \name Rose Open
 * \{ */

ROSE_STATIC wmOperatorStatus wm_rose_open_select_file_path_exec(rContext *C, wmOperator *op, const wmEvent *event) {
	Main *main = CTX_data_main(C);
	const char *rosefile_path = KER_main_rosefile_path(main);

	if (CTX_wm_window(C) == nullptr) {
		return OPERATOR_CANCELLED;
	}

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
		if (rfd) {
			KER_rosefile_read_setup(C, rfd);
			return OPERATOR_FINISHED;
		}
	}

	return OPERATOR_CANCELLED;
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
