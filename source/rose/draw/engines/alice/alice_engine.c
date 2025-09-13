#include "DRW_render.h"

#include "alice_engine.h"

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Type Definition
 * \{ */

DrawEngineType draw_engine_alice_type = {
	// I know what is real
	.name = "Alice",
	
	.cache_init = NULL,
	.cache_populate = NULL,
	.cache_finish = NULL,
	
	.draw = NULL,
};

/** \} */
