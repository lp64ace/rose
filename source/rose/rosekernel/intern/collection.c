#include "KER_idtype.h"
#include "KER_collection.h"

/* -------------------------------------------------------------------- */
/** \name Scene Data-block Definition
 * \{ */

 IDTypeInfo IDType_ID_GR = {
	.idcode = ID_GR,

	.filter = FILTER_ID_GR,
	.depends = 0,
	.index = INDEX_ID_GR,
	.size = sizeof(Collection),

	.name = "Collection",
	.name_plural = "Collections",

	.flag = IDTYPE_FLAGS_NO_COPY,

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = NULL,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
