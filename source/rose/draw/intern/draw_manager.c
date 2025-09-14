#include "DRW_engine.h"
#include "DRW_render.h"

#include "engines/alice/alice_engine.h"
#include "engines/basic/basic_engine.h"

static ListBase registered_engine_list;

void DRW_engines_register(void) {
	DRW_engine_register(&draw_engine_basic_type);
	DRW_engine_register(&draw_engine_alice_type);
}

void DRW_engines_register_experimental(void) {
}

void DRW_engines_free(void) {
	LISTBASE_FOREACH_MUTABLE(DrawEngineType *, engine, &registered_engine_list) {
		
	}
	LIB_listbase_clear(&registered_engine_list);
}

void DRW_engine_register(DrawEngineType *draw_engine_type) {
	LIB_addtail(&registered_engine_list, draw_engine_type);
}
