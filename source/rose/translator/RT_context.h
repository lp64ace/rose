#ifndef RT_CONTEXT_H
#define RT_CONTEXT_H

#include "LIB_memarena.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct RCContext {
	/**
	 * Every object is allocated using this allocator and everything is deleted when
	 * the lifetime of this context expires.
	 *
	 * Types, Nodes, Objects are allocated using this, that way they remain valid throughout
	 * the compilation of a source bundle.
	 */
	struct MemArena *alloc;
} RCContext;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

RCContext *RT_context_new();

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RT_context_free(RCContext *);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void *RT_context_malloc(RCContext *, size_t length);
void *RT_context_calloc(RCContext *, size_t length);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_CONTEXT_H
