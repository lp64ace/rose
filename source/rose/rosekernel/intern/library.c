#include "LIB_path_utils.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_idtype.h"
#include "KER_library.h"

#include "DNA_ID.h"
#include "DNA_ID_enums.h"

/* -------------------------------------------------------------------- */
/** \name Path Methods
 * \{ */

bool KER_library_filepath_set(struct Main *main, struct Library *lib, const char *filepath) {
	if (LIB_path_absolute(lib->filepath, ARRAY_SIZE(lib->filepath), filepath)) {
		return true;
	}
	LIB_strcpy(lib->filepath, ARRAY_SIZE(lib->filepath), filepath);
	return false;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Library Data-block definition
 * \{ */

IDTypeInfo IDType_ID_LI = {
	.idcode = ID_LI,

	.filter = FILTER_ID_LI,
	.depends = 0,
	.index = INDEX_ID_LI,
	.size = sizeof(Library),

	.name = "Library",
	.name_plural = "Libraries",

	.flag = IDTYPE_FLAGS_NO_COPY | IDTYPE_FLAGS_NO_ANIMDATA,

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = NULL,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
