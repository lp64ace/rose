#include "MEM_guardedalloc.h"

#include "LIB_assert.h"
#include "LIB_memarena.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct MemBuf {
	struct MemBuf *next;

	unsigned char data[];
} MemBuf;

typedef struct MemArena {
	MemBuf *buffers;

	unsigned char *itr;

	size_t bufsize;
	size_t cursize;

	const char *name;
} MemArena;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Arena Creation
 * \{ */

MemArena *LIB_memory_arena_create(size_t size, const char *name) {
	MemArena *arena = MEM_callocN(sizeof(MemArena), name);

	arena->bufsize = size;
	arena->name = name;

	return arena;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Arena Destruction
 * \{ */

ROSE_INLINE void memory_arena_free_all(MemArena *arena, MemBuf *iter) {
	while (iter) {
		MemBuf *next = iter->next;
		MEM_freeN(iter);
		iter = next;
	}
}

void LIB_memory_arena_destroy(MemArena *arena) {
	memory_arena_free_all(arena, arena->buffers);
	MEM_freeN(arena);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Memory Allocation and Deallocation
 * \{ */

/** Pad num up by \a amt (must be power of two). */
#define PADUP(num, amt) (((num) + ((amt) - 1)) & ~((amt) - 1))

void *LIB_memory_arena_malloc(MemArena *arena, size_t size) {
	void *ptr = NULL;

	ROSE_assert(size < arena->bufsize);
	if (size > arena->cursize) {
		arena->cursize = arena->bufsize;

		MemBuf *nbuf = MEM_mallocN(arena->bufsize + sizeof(MemBuf), arena->name);

		/** Emplace this new buffer at the beginning of the list. */
		nbuf->next = arena->buffers;
		arena->buffers = nbuf;
		arena->itr = nbuf->data;
	}

	ptr = arena->itr;

	arena->itr = POINTER_OFFSET(arena->itr, size);
	arena->cursize -= size;

	return ptr;
}

void *LIB_memory_arena_calloc(MemArena *arena, size_t size) {
	void *ptr = LIB_memory_arena_malloc(arena, size);

	memset(ptr, 0, size);

	return ptr;
}

void *LIB_memory_arena_strdup(MemArena *arena, const char *str) {
	const size_t length = LIB_strlen(str);
	void *ptr = LIB_memory_arena_malloc(arena, length + 1);
	return LIB_strcpy(ptr, length + 1, str);
}

void *LIB_memory_arena_strfmt(MemArena *arena, const char *fmt, ...) {
	va_list args;
	char *str;

	va_start(args, fmt);
	str = LIB_vstrformat_allocN(fmt, args);
	va_end(args);

	void *ptr = NULL;
	if (str) {
		ptr = LIB_memory_arena_strdup(arena, str);
		MEM_freeN(str);
	}

	return ptr;
}

void LIB_memory_arena_clear(MemArena *arena) {
	MemBuf *cbuf = arena->buffers;

	if (cbuf) {
		/** Free all the buffers except the first one. */
		memory_arena_free_all(arena, cbuf->next);

#ifndef NDEBUG
		memset(arena, 0xff, arena->bufsize);
#endif

		/** Reset the offset inside the current buffer. */
		arena->cursize = arena->bufsize;
		arena->itr = cbuf->data;
	}
}

/** \} */
