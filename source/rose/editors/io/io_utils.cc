#include "DNA_space_types.h"

#include "KER_main.h"

#include "LIB_fileops.h"
#include "LIB_path_utils.h"
#include "LIB_utildefines.h"

#include "RNA_access.h"
#include "RNA_prototypes.h"

#include "io_utils.hh"

namespace rose::editors::io {

wmOperatorStatus filesel_drop_import_invoke(rContext *C, wmOperator *op, const wmEvent *event) {
	PropertyRNA *filepath_prop = RNA_struct_find_property(op->ptr, "filepath");
	PropertyRNA *directory_prop = RNA_struct_find_property(op->ptr, "directory");
	if ((filepath_prop && RNA_property_is_set(op->ptr, filepath_prop)) || (directory_prop && RNA_property_is_set(op->ptr, directory_prop))) {
		std::string title;
		PropertyRNA *files_prop = RNA_struct_find_property(op->ptr, "files");
		if (directory_prop && files_prop) {
			const auto files = paths_from_operator_properties(op->ptr);
			if (files.size() == 1) {
				title = files[0];
			}
			else {
				title = "Import " + std::to_string(files.size()) + " files";
			}
		}
		else {
			char filepath[FILE_MAX];
			RNA_string_get(op->ptr, "filepath", filepath);
			title = filepath;
		}

		const char *ctitle = &title[0];
		const char *otname = WM_operatortype_name(op->type, op->ptr);

		// return WM_operator_props_dialog_popup(C, op, 350, ctitle, otname, false, NULL);
	}

	WM_event_add_fileselect(C, op);
	return OPERATOR_RUNNING_MODAL;
}

rose::Vector<std::string> paths_from_operator_properties(struct PointerRNA *ptr) {
	rose::Vector<std::string> paths;
	PropertyRNA *directory_property = RNA_struct_find_property(ptr, "directory");
	PropertyRNA *relative_path_property = RNA_struct_find_property(ptr, "relative_path");
	const bool is_relative_path = relative_path_property ? RNA_property_boolean_get(ptr, relative_path_property) : false;

	if (RNA_property_is_set(ptr, directory_property)) {
		char directory[FILE_MAX], name[FILE_MAX];

		RNA_property_string_get(ptr, directory_property, directory);
		if (is_relative_path && !LIB_path_is_rel(directory)) {
			LIB_path_rel(directory, KER_main_rosefile_path_from_global());
		}

#if 0
		PropertyRNA *files_property = RNA_struct_find_collection_property_check(ptr, "files", &RNA_OperatorFileListElement);
		ROSE_assert(files_property);

		RNA_PROP_BEGIN(ptr, file_ptr, files_property) {
			RNA_string_get(&file_ptr, "name", name);
			char path[FILE_MAX];
			LIB_path_join(path, sizeof(path), directory, name);
			LIB_path_normalize(path);
			paths.append_non_duplicates(path);
		}
		RNA_PROP_END;
#endif
	}
	PropertyRNA *filepath_prop = RNA_struct_find_property(ptr, "filepath");
	if (filepath_prop && RNA_property_is_set(ptr, filepath_prop)) {
		char filepath[FILE_MAX];
		RNA_string_get(ptr, "filepath", filepath);
		if (is_relative_path && !LIB_path_is_rel(filepath)) {
			LIB_path_rel(filepath, KER_main_rosefile_path_from_global());
		}
		paths.append_non_duplicates(filepath);
	}
	return paths;
}

}  // namespace rose::editors::io
