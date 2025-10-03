#include "MEM_guardedalloc.h"

#include "DRW_cache.h"
#include "DRW_engine.h"
#include "DRW_render.h"

#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include "draw_engine.h"
#include "draw_manager.h"

ROSE_STATIC void *draw_command_new(DRWShadingGroup *shgroup, int type) {
	DRWCommand *cmd = LIB_memory_pool_calloc(GDrawManager.vdata_pool->commands);

	cmd->type = type;

	LIB_addtail(&shgroup->commands, cmd);

	return (void *)&cmd->draw;
}

ROSE_STATIC void draw_command_draw(DRWShadingGroup *shgroup, struct GPUBatch *batch, unsigned int vcount) {
	DRWCommandDraw *cmd = draw_command_new(shgroup, DRW_COMMAND_DRAW);

	cmd->batch = batch;
	cmd->vcount = vcount;
}

ROSE_STATIC void draw_command_draw_range(DRWShadingGroup *shgroup, struct GPUBatch *batch, unsigned int vfirst, unsigned int vcount) {
	DRWCommandDrawRange *cmd = draw_command_new(shgroup, DRW_COMMAND_DRAW_RANGE);

	cmd->batch = batch;
	cmd->vfirst = vfirst;
	cmd->vcount = vcount;
}

void DRW_shading_group_call_ex(DRWShadingGroup *shgroup, struct GPUBatch *batch) {
	draw_command_draw(shgroup, batch, 0);
}

void DRW_shading_group_call_range_ex(DRWShadingGroup *shgroup, struct GPUBatch *batch, unsigned int vfirst, unsigned int vcount) {
	draw_command_draw_range(shgroup, batch, vfirst, vcount);
}

ROSE_STATIC DRWShadingGroup *draw_shading_group_new_ex(GPUShader *shader, DRWPass *pass) {
	DRWShadingGroup *shgroup = LIB_memory_pool_calloc(GDrawManager.vdata_pool->shgroups);

	shgroup->shader = shader;
	shgroup->commands.first = NULL;
	shgroup->commands.last = NULL;
	shgroup->uniforms.first = NULL;
	shgroup->uniforms.last = NULL;

	LIB_addtail(&pass->groups, shgroup);

	return shgroup;
}

/**
 * Builtin uniforms should be handled here, when applicable!
 */
ROSE_STATIC void draw_shading_group_init(DRWShadingGroup *shgroup) {
}

DRWShadingGroup *DRW_shading_group_new(GPUShader *shader, DRWPass *pass) {
	DRWShadingGroup *shgroup = draw_shading_group_new_ex(shader, pass);
	draw_shading_group_init(shgroup);
	return shgroup;
}
