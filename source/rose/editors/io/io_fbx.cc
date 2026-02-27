#include "MEM_guardedalloc.h"

#include "KER_context.h"

#include "LIB_vector.hh"

#include "IO_fbx.h"

#include "WM_api.h"
#include "WM_handler.h"

#include "io_utils.hh"

/* -------------------------------------------------------------------- */
/** \name IO FBX Operators
 * \{ */

ROSE_STATIC wmOperatorStatus wm_fbx_import_exec(rContext *C, wmOperator *op) {
	rose::Vector<std::string> paths = rose::editors::io::paths_from_operator_properties(op->ptr);
	if (paths.is_empty()) {
		return OPERATOR_CANCELLED;
	}

	for (const std::string &path : paths) {
		FBX_import(C, &path[0], 1.0f);
	}

	return OPERATOR_FINISHED;
}

ROSE_STATIC bool wm_fbx_import_check(rContext *C, wmOperator *op) {
	return false;
}

ROSE_STATIC bool wm_fbx_import_poll(rContext *C) {
	if (!CTX_wm_window(C)) {
		return false;
	}
	return true;
}

void WM_OT_fbx_import(wmOperatorType *ot) {
	ot->name = "Import FBX";
	ot->description = "Import FBX file into current scene";
	ot->idname = "WM_OT_fbx_import";

	ot->invoke = rose::editors::io::filesel_drop_import_invoke;
	ot->exec = wm_fbx_import_exec;
	ot->check = wm_fbx_import_check;
	ot->poll = wm_fbx_import_poll;

	WM_operator_properties_filesel(ot, FILE_TYPE_FOLDER, FILE_ROSE, FILE_OPENFILE, WM_FILESEL_FILEPATH | WM_FILESEL_DIRECTORY | WM_FILESEL_FILES);
}

/** \} */
