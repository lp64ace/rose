#include "MEM_guardedalloc.h"

#include "COM_context.h"
#include "COM_token.h"
#include "COM_type.h"

Compilation *COM_new(void) {
	Compilation *com = MEM_mallocN(sizeof(Compilation), "RoseC");

	com->node = LIB_memory_pool_create(1, 0xffu, 0xffu, ROSE_MEMPOOL_NOP);
	com->decl = LIB_memory_pool_create(1, 0xffu, 0xffu, ROSE_MEMPOOL_NOP);
	com->scope = LIB_memory_pool_create(1, 0xffu, 0xffu, ROSE_MEMPOOL_NOP);
	com->token = LIB_memory_pool_create(sizeof(CToken), 0xffu, 0xffu, ROSE_MEMPOOL_NOP);
	com->type = LIB_memory_pool_create(sizeof(CType), 0xffu, 0xffu, ROSE_MEMPOOL_NOP);

	com->blob = LIB_memory_arena_create(0xffffu, "Blob");

	LIB_listbase_clear(&com->tokens);
	LIB_listbase_clear(&com->nodes);

	return com;
}

void COM_free(Compilation *com) {
	LIB_memory_pool_destroy(com->node);
	LIB_memory_pool_destroy(com->decl);
	LIB_memory_pool_destroy(com->scope);
	LIB_memory_pool_destroy(com->token);
	LIB_memory_pool_destroy(com->type);
	LIB_memory_arena_destroy(com->blob);
	MEM_freeN(com);
}
