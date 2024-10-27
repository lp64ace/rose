#include "MEM_guardedalloc.h"

#include "RT_context.h"

/** We use this to allocated space for source files, support up to 16MB source files. */
#define MEMARENA_ALLOCATION_BLOCK 1024 * 1024 * 16

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

RCContext *RT_context_new() {
	RCContext *c = MEM_callocN(sizeof(RCContext), "RCContext");
	c->alloc = LIB_memory_arena_create(MEMARENA_ALLOCATION_BLOCK, "RCCItem");
	return c;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RT_context_free(RCContext *c) {
	LIB_memory_arena_destroy(c->alloc);
	MEM_freeN(c);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void *RT_context_malloc(RCContext *cc, size_t length) {
	return LIB_memory_arena_malloc(cc->alloc, length);
}
void *RT_context_calloc(RCContext *cc, size_t length) {
	return LIB_memory_arena_calloc(cc->alloc, length);
}

/** \} */
