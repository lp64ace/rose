#include "DNA_ID.h"

#include "LIB_assert.h"
#include "LIB_utildefines.h"

#include "KER_idtype.h"
#include "KER_library.h"

static void library_free_data(struct ID *id) {
	struct Library *library = (struct Library *)id;

	if (library->runtime.name_map) {
		ROSE_assert_unreachable();
	}
}

IDTypeInfo IDType_ID_LI = {
	.id_code = ID_LI,
	.id_filter = FILTER_ID_LI,
	.main_listbase_index = INDEX_ID_LI,

	.struct_size = sizeof(Library),
	.name = "Library",
	.name_plural = "Libraries",

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = library_free_data,
};
