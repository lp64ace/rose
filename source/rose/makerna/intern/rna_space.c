#include "DNA_object_types.h"

#include "RNA_access.h"
#include "RNA_define.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

static void rna_def_fileselect_idfilter(RoseRNA *rna) {
	StructRNA *fileselectparams = RNA_def_struct(rna, "FileSelectParams", NULL);
	RNA_def_struct_sdna(fileselectparams, "FileSelectParams");
	RNA_def_struct_ui_text(fileselectparams, "File Select ID Filter", "Which ID types to show/hide, when browsing a library");
}

void RNA_def_space(RoseRNA *rna) {
	rna_def_fileselect_idfilter(rna);
}
