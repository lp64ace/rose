#include "MEM_guardedalloc.h"

#include "GPU_batch.h"
#include "GPU_uniform_buffer.h"

#include "KER_modifier.h"
#include "KER_object.h"

#include "alice_engine.h"
#include "alice_private.h"

#include "DRW_engine.h"

#include "intern/draw_defines.h"
#include "intern/draw_manager.h"
#include "intern/draw_pass.h"
#include "intern/draw_state.h"

#include "intern/mesh/extract_mesh.h"
#include "intern/shaders/draw_shader_shared.h"

ROSE_INLINE void alice_draw_data_init(DrawData *dd) {
    AliceDrawData *add = (AliceDrawData *)dd;

	add->flag |= ALICE_SHADOW_BOX_DIRTY;
}

ROSE_INLINE void alice_draw_data_free(DrawData *dd) {
    AliceDrawData *add = (AliceDrawData *)dd;
}

AliceDrawData *DRW_alice_drawdata(Object *object) {
	AliceDrawData *add = (AliceDrawData *)KER_drawdata_ensure(&object->id, &draw_engine_alice_type, sizeof(AliceDrawData), alice_draw_data_init, alice_draw_data_free);

	return add;
}
