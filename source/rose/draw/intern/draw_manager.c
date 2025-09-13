#include "DRW_engine.h"

#include "engines/alice/alice_engine.h"
#include "engines/basic/basic_engine.h"

void DRW_engines_register(void) {
	DRW_engine_register(&draw_engine_basic_type);
	DRW_engine_register(&draw_engine_alice_type);
}

void DRW_engines_register_experimental(void) {
}

void DRW_engines_free(void) {
}
