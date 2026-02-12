#include "DNA_space_types.h"

#include "RNA_access.h"

#include "LIB_string.h"
#include "LIB_path_utils.h"

#include "WM_api.h"
#include "WM_handler.h"

#include "file.h"

/* -------------------------------------------------------------------- */
/** \name File Select Params
 * \{ */

ROSE_INLINE void fileselect_initialize_params_common(SpaceFile *sfile, FileSelectParams *params) {
	if (!params->dir[0]) {
		LIB_path_current_working_directory(params->dir, ARRAY_SIZE(params->dir));
	}
}

FileSelectParams *ED_fileselect_ensure_active_params(SpaceFile *file) {
	FileSelectParams *params;
	wmOperator *op = file->op;

	if (!file->params) {
		file->params = MEM_callocN(sizeof(FileSelectParams), "FileSelectParams");

		LIB_path_current_working_directory(file->params->dir, ARRAY_SIZE(file->params->dir));
	}

	params = file->params;

	if (op) {
		PropertyRNA *prop;
		const bool is_files = (RNA_struct_find_property(op->ptr, "files") != NULL);
		const bool is_filepath = (RNA_struct_find_property(op->ptr, "filepath") != NULL);
		const bool is_filename = (RNA_struct_find_property(op->ptr, "filename") != NULL);
		const bool is_directory = (RNA_struct_find_property(op->ptr, "directory") != NULL);
		const bool is_relative_path = (RNA_struct_find_property(op->ptr, "relative_path") != NULL);

		LIB_strcpy(params->title, op->type->name, sizeof(params->title));

		if (is_filepath && RNA_struct_property_is_set_ex(op->ptr, "filepath")) {
			char filepath[FILE_MAX];
			RNA_string_get(op->ptr, "filepath", filepath);
		}
		else {
			if (is_directory && RNA_struct_property_is_set_ex(op->ptr, "directory")) {
				RNA_string_get(op->ptr, "directory", params->dir);
				params->file[0] = '\0';
			}

			if (is_filename && RNA_struct_property_is_set_ex(op->ptr, "filename")) {
				RNA_string_get(op->ptr, "filename", params->file);
			}
		}

		if (params->dir[0]) {
			LIB_path_current_working_directory(file->params->dir, ARRAY_SIZE(file->params->dir));
		}

		params->flag = 0;
		if (is_directory == true && is_filename == false && is_filepath == false && is_files == false) {
			params->flag |= FILE_DIRSEL_ONLY;
		}
		if ((prop = RNA_struct_find_property(op->ptr, "check_existing"))) {
			params->flag |= RNA_property_boolean_get(op->ptr, prop) ? FILE_CHECK_EXISTING : 0;
		}
	}

	fileselect_initialize_params_common(file, params);

	return params;
}

FileSelectParams *ED_fileselect_get_active_params(SpaceFile *file) {
	if (!file) {
		/* Sometimes called in poll before space type was checked. */
		return NULL;
	}

	return file->params;
}

/** \} */
