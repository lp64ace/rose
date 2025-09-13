#include "DRW_render.h"

#include "basic_engine.h"

/* -------------------------------------------------------------------- */
/** \name Basic Draw Engine Type Definition
 * \{ */

DrawEngineType draw_engine_basic_type = {
	.name = "Rose",
	
	.cache_init = NULL,
	.cache_populate = NULL,
	.cache_finish = NULL,
	
	.draw = NULL,
};

/** \} */
