#include "DRW_render.h"

#include "alice_engine.h"

/* -------------------------------------------------------------------- */
/** \name Alice Draw Engine Type Definition
 * \{ */

DrawEngineType draw_engine_alice_type = {
	.name = "ROSE_ALICE", // I know what is real
	
	.cache_init = NULL,
	.cache_populate = NULL,
	.cache_finish = NULL,
	
	.draw = NULL,
};

/** \} */
