#include "DNA_object_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

static void rna_def_fileselect_params(RoseRNA *rna) {
	StructRNA *fileselectparams = RNA_def_struct(rna, "FileSelectParams", NULL);
	RNA_def_struct_sdna(fileselectparams, "FileSelectParams");
	RNA_def_struct_ui_text(fileselectparams, "File Select Parameters", "File Select Parameters");

	do {
		PropertyRNA *title = RNA_def_property(fileselectparams, "title", PROP_STRING, PROP_NONE);
		RNA_def_property_string_sdna(title, NULL, "title");
		RNA_def_property_ui_text(title, "Title", "Title for the file browser");
		RNA_def_property_clear_flag(title, PROP_EDITABLE);
	} while (false);

	do {
		PropertyRNA *directory = RNA_def_property(fileselectparams, "directory", PROP_STRING, PROP_NONE);
		RNA_def_property_string_sdna(directory, NULL, "dir");
		RNA_def_property_ui_text(directory, "Directory", "Directory displayed in the file browser");
	} while (false);

	do {
		PropertyRNA *filename = RNA_def_property(fileselectparams, "filename", PROP_STRING, PROP_FILENAME);
		RNA_def_property_string_sdna(filename, NULL, "file");
		RNA_def_property_ui_text(filename, "File Name", "Active file in the file browser");
	} while (false);
}

void RNA_def_space(RoseRNA *rna) {
	rna_def_fileselect_params(rna);
}
