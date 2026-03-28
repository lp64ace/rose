#include "MEM_guardedalloc.h"

#include "KER_context.h"
#include "KER_rosefile.h"

#include "LIB_vector.hh"

#include "IO_fbx.h"

#include "WM_api.h"
#include "WM_handler.h"

#include "io_utils.hh"

/* -------------------------------------------------------------------- */
/** \name Rose Open
 * \{ */

ROSE_STATIC wmOperatorStatus wm_rose_open_exec(rContext *C, wmOperator *op) {
	rose::Vector<std::string> paths = rose::editors::io::paths_from_operator_properties(op->ptr);
	if (paths.is_empty()) {
		return OPERATOR_CANCELLED;
	}

	for (const std::string &path : paths) {
		RoseFileData *rfd = KER_rosefile_read(&path[0], 0);
		KER_rosefile_read_setup(C, rfd);
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

void WM_OT_rose_open(wmOperatorType *ot) {
	ot->name = "Open";
	ot->description = "Open rose file from disk";
	ot->idname = "WM_OT_rose_open";

	ot->invoke = rose::editors::io::filesel_drop_import_invoke;
	ot->exec = wm_rose_open_exec;
	ot->check = wm_rose_open_check;
	ot->poll = wm_rose_open_poll;

	WM_operator_properties_filesel(ot, FILE_TYPE_FOLDER, FILE_ROSE, FILE_OPENFILE, WM_FILESEL_FILEPATH | WM_FILESEL_DIRECTORY | WM_FILESEL_FILES);
}

/** \} */
