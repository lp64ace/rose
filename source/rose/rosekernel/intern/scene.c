#include "KER_idtype.h"
#include "KER_scene.h"

/* -------------------------------------------------------------------- */
/** \name Object Data-block Definition
 * \{ */

 IDTypeInfo IDType_ID_SCE = {
	.idcode = ID_SCE,

	.filter = FILTER_ID_SCE,
	.depends = 0,
	.index = INDEX_ID_SCE,
	.size = sizeof(Scene),

	.name = "Scene",
	.name_plural = "Scenes",

	.flag = IDTYPE_FLAGS_NO_COPY,

	.init_data = NULL,
	.copy_data = NULL,
	.free_data = NULL,

	.foreach_id = NULL,

	.write = NULL,
	.read_data = NULL,
};

/** \} */

