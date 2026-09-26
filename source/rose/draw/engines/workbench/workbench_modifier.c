#include "MEM_guardedalloc.h"

#include "GPU_batch.h"
#include "GPU_uniform_buffer.h"

#include "KER_modifier.h"
#include "KER_object.h"

#include "workbench_engine.h"
#include "workbench_private.h"

#include "DRW_engine.h"

#include "intern/draw_defines.h"
#include "intern/draw_manager.h"
#include "intern/draw_pass.h"
#include "intern/draw_state.h"

#include "intern/mesh/extract_mesh.h"
#include "intern/shaders/draw_shader_shared.h"

ROSE_INLINE void workbench_draw_data_init(DrawData *dd) {
    WorkbenchDrawData *add = (WorkbenchDrawData *)dd;

	add->flag |= WORKBENCH_SHADOW_BOX_DIRTY;
}

ROSE_INLINE void workbench_draw_data_free(DrawData *dd) {
    WorkbenchDrawData *add = (WorkbenchDrawData *)dd;
}

WorkbenchDrawData *DRW_workbench_drawdata(Object *object) {
	WorkbenchDrawData *add = (WorkbenchDrawData *)KER_drawdata_ensure(&object->id, &draw_engine_workbench_type, sizeof(WorkbenchDrawData), workbench_draw_data_init, workbench_draw_data_free);

	return add;
}
