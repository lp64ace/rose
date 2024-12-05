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

typedef struct RTContext {
	/**
	 * Every object is allocated using this allocator and everything is deleted when
	 * the lifetime of this context expires.
	 *
	 * Types, Nodes, Objects are allocated using this, that way they remain valid throughout
	 * the compilation of a source bundle.
	 */
	struct MemArena *alloc;
} RTContext;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Create Methods
 * \{ */

RTContext *RT_context_new();

/** \} */

/* -------------------------------------------------------------------- */
/** \name Delete Methods
 * \{ */

void RT_context_free(RTContext *);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void *RT_context_malloc(RTContext *, size_t length);
void *RT_context_calloc(RTContext *, size_t length);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RT_CONTEXT_H
