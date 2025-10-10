#include "MEM_guardedalloc.h"

#include "LIB_assert.h"
#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include "atomic_ops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __BIG_ENDIAN__
/* Big Endian */
#	define MAKE_ID4(a, b, c, d) ((int)(a) << 24 | (int)(b) << 16 | (c) << 8 | (d))
#	define MAKE_ID8(a, b, c, d, e, f, g, h) ((int64_t)(a) << 56 | (int64_t)(b) << 48 | (int64_t)(c) << 40 | (int64_t)(d) << 32 | (int64_t)(e) << 24 | (int64_t)(f) << 16 | (int64_t)(g) << 8 | (h))
#else
/* Little Endian */
#	define MAKE_ID4(a, b, c, d) ((int)(d) << 24 | (int)(c) << 16 | (b) << 8 | (a))
#	define MAKE_ID8(a, b, c, d, e, f, g, h) ((int64_t)(h) << 56 | (int64_t)(g) << 48 | (int64_t)(f) << 40 | (int64_t)(e) << 32 | (int64_t)(d) << 24 | (int64_t)(c) << 16 | (int64_t)(b) << 8 | (a))
#endif

#define USEDWORD MAKE_ID4('u', 's', 'e', 'd')
/**
 * Important that this value is an is _not_  aligned with `sizeof(void *)`.
 * So having a pointer to 2/4/8... aligned memory is enough to ensure
 * the `freeword` will never be used.
 * To be safe, use a word that's the same in both directions.
 */
#define FREEWORD ((sizeof(void *) > sizeof(int32_t)) ? MAKE_ID8('e', 'e', 'r', 'f', 'f', 'r', 'e', 'e') : MAKE_ID4('e', 'f', 'f', 'e'))

typedef struct FreeNode {
	/** Each element represents a block which `LIB_memory_pool_malloc` may return. */
	struct FreeNode *next;
	/** Used to identify this as a freed node. */
	intptr_t freeword;
} FreeNode;

typedef struct PoolChunk {
	/** A chunk of memory in the mempool stored in #chunks_head as a single-linked list. */
	struct PoolChunk *next;
} PoolChunk;

typedef struct MemPool {
	PoolChunk *chunks_head;
	PoolChunk *chunks_tail;

	size_t esize;
	size_t csize;
	size_t clength;

	int flag;

	FreeNode *free;

	/** The length of the pool, in elements, the currently active elements. */
	size_t length;
} MemPool;

#define MEMPOOL_ELEM_SIZE_MIN (sizeof(FreeNode))
#define CHUNK_DATA(chunk) ((void *)(((PoolChunk *)chunk) + 1))

#define NODE_STEP_NEXT(node) ((void *)((char *)(node) + esize))
#define NODE_STEP_PREV(node) ((void *)((char *)(node) - esize))

/* -------------------------------------------------------------------- */
/** \name Internal Utilities
 * \{ */

ROSE_INLINE PoolChunk *memory_pool_chunk_find(PoolChunk *head, size_t index) {
	while (index-- && head) {
		head = head->next;
	}
	return head;
}

ROSE_INLINE size_t memory_pool_num_chunks(size_t length, size_t perchunk) {
	return (length <= perchunk) ? 1 : ((length / perchunk) + 1);
}

ROSE_INLINE PoolChunk *memory_pool_chunk_new(MemPool *pool) {
	return MEM_mallocN(sizeof(PoolChunk) + pool->csize, "mempool::chunk");
}
ROSE_INLINE void memory_pool_chunk_free(MemPool *pool, PoolChunk *chunk) {
	UNUSED_VARS(pool);
	MEM_SAFE_FREE(chunk);
}
ROSE_INLINE void memory_pool_chunk_free_all(MemPool *pool, PoolChunk *chunk) {
	for (PoolChunk *next; chunk; chunk = next) {
		next = chunk->next;
		memory_pool_chunk_free(pool, chunk);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Chunk Management
 * \{ */

FreeNode *memory_pool_chunk_add(MemPool *pool, PoolChunk *chunk, FreeNode *last) {
	const size_t esize = pool->esize;
	FreeNode *node = CHUNK_DATA(chunk);

	if (pool->chunks_tail) {
		pool->chunks_tail->next = chunk;
	}
	else {
		pool->chunks_head = chunk;
	}

	chunk->next = NULL;
	pool->chunks_tail = chunk;

	if (pool->free == NULL) {
		pool->free = node;
	}

	size_t i = pool->clength;
	if (pool->flag & ROSE_MEMPOOL_ALLOW_ITER) {
		while (i--) {
			FreeNode *next;

			node->next = next = NODE_STEP_NEXT(node);
			node->freeword = FREEWORD;

			node = next;
		}
	}
	else {
		while (i--) {
			FreeNode *next;

			node->next = next = NODE_STEP_NEXT(node);

			node = next;
		}
	}

	node = NODE_STEP_PREV(node);

	if (last) {
		last->next = CHUNK_DATA(chunk);
	}

	node->next = NULL;

	return node;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pool Creation
 * \{ */

MemPool *LIB_memory_pool_create(size_t element, size_t perchunk, size_t reserve, int flag) {
	MemPool *pool = MEM_mallocN(sizeof(MemPool), "mempool");

	element = ROSE_MAX(element, MEMPOOL_ELEM_SIZE_MIN);

	pool->chunks_head = NULL;
	pool->chunks_tail = NULL;

	pool->esize = element;
	pool->clength = perchunk;
	pool->csize = pool->clength * pool->esize;

	pool->free = NULL;

	pool->flag = flag;
	pool->length = 0;

	if (reserve) {
		FreeNode *last = NULL;
		for (size_t i = 0; i * pool->clength < reserve; i++) {
			last = memory_pool_chunk_add(pool, memory_pool_chunk_new(pool), last);
		}
	}

	return pool;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pool Destruction
 * \{ */

void LIB_memory_pool_destroy(MemPool *pool) {
	memory_pool_chunk_free_all(pool, pool->chunks_head);

	MEM_freeN(pool);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Memory Allocation and Deallocation
 * \{ */

void *LIB_memory_pool_malloc(MemPool *pool) {
	FreeNode *free_pop;

	if (pool->free == NULL) {
		memory_pool_chunk_add(pool, memory_pool_chunk_new(pool), NULL);
		ROSE_assert(pool->free);
	}

	if ((free_pop = pool->free)) {
		free_pop->freeword = USEDWORD;

		ROSE_assert(pool->chunks_tail->next == NULL);

		pool->free = free_pop->next;
		pool->length++;
	}

	return (void *)free_pop;
}

void *LIB_memory_pool_calloc(MemPool *pool) {
	void *ptr = LIB_memory_pool_malloc(pool);
	if (ptr) {
		memset(ptr, 0, pool->esize);
	}
	return ptr;
}

void LIB_memory_pool_free(MemPool *pool, void *ptr) {
	FreeNode *newhead = ptr;

#ifndef NDEBUG
	{
		bool found = false;
		for (PoolChunk *chunk = pool->chunks_head; chunk; chunk = chunk->next) {
			if (ARRAY_HAS_ITEM((char *)ptr, (char *)CHUNK_DATA(chunk), pool->csize)) {
				found |= true;
			}
		}
		if (!found) {
			ROSE_assert_msg(found, "Attempt to free data which is not in pool.\n");
		}
	}
#endif

	newhead->freeword = FREEWORD;

	newhead->next = pool->free;
	pool->free = newhead;

	pool->length--;

	size_t esize = pool->esize;
	if (pool->length == 0) {
		pool->free = CHUNK_DATA(pool->chunks_head);

		memory_pool_chunk_free_all(pool, pool->chunks_head->next);
		pool->chunks_head->next = NULL;
		pool->chunks_tail = pool->chunks_head;

		size_t i = pool->clength;
		while (i--) {
			FreeNode *next;

			pool->free->next = next = NODE_STEP_NEXT(pool->free);
			pool->free->freeword = FREEWORD;

			pool->free = next;
		}

		pool->free = NODE_STEP_PREV(pool->free);
		pool->free->next = NULL;
	}
}

void LIB_memory_pool_clear(MemPool *pool, size_t reserve) {
	size_t nchunks = memory_pool_num_chunks(reserve, pool->clength);

	PoolChunk *chunk = memory_pool_chunk_find(pool->chunks_head, nchunks - 1);
	if (chunk && chunk->next) {
		PoolChunk *next = chunk->next;
		chunk->next = NULL;
		chunk = next;

		do {
			next = chunk->next;
			memory_pool_chunk_free(pool, chunk);
			chunk = next;
		} while (chunk);
	}
	chunk = pool->chunks_head;

	pool->chunks_head = NULL;
	pool->chunks_tail = NULL;

	pool->free = NULL;

	pool->length = 0;

	FreeNode *last = NULL;
	while (chunk) {
		last = memory_pool_chunk_add(pool, chunk, last);
		chunk = chunk->next;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Statistics
 * \{ */

size_t LIB_memory_pool_length(MemPool *pool) {
	return pool->length;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

void *LIB_memory_pool_findelem(MemPool *pool, size_t index) {
	ROSE_assert(pool->flag & ROSE_MEMPOOL_ALLOW_ITER);

	if (index < pool->length) {
		/* We could have some faster mem chunk stepping code inline. */
		MemPoolIter iter;
		void *elem;
		LIB_memory_pool_iternew(pool, &iter);
		for (elem = LIB_memory_pool_iterstep(&iter); index-- != 0; elem = LIB_memory_pool_iterstep(&iter)) {
			/* pass */
		}
		return elem;
	}
	return NULL;
}

void *LIB_memory_pool_findelem_ex(MemPool *pool, size_t ichunk, size_t ielem) {
	PoolChunk *chunk = memory_pool_chunk_find(pool->chunks_head, ichunk);

	FreeNode *ret = NULL;

	size_t index = 0;
	do {
		FreeNode *curnode = POINTER_OFFSET(CHUNK_DATA(chunk), (pool->esize * index));

		do {
			ret = curnode;

			if (++index != pool->clength) {
				curnode = POINTER_OFFSET(curnode, pool->esize);
			}
			else {
				/**
				 * We are searching for an element that does not exist within this memory pool chunk!
				 */
				ROSE_assert_unreachable();
			}
		} while (ret->freeword == FREEWORD);
	} while (ielem--);

	return ret;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Iterator
 * \{ */

void LIB_memory_pool_iternew(MemPool *pool, MemPoolIter *iter) {
	ROSE_assert(pool->flag & ROSE_MEMPOOL_ALLOW_ITER);

	iter->pool = pool;
	iter->chunk = pool->chunks_head;
	iter->index = 0;
}

void *LIB_memory_pool_iterstep(MemPoolIter *iter) {
	if (iter->chunk == NULL) {
		return NULL;
	}

	const uint esize = iter->pool->esize;
	FreeNode *curnode = POINTER_OFFSET(CHUNK_DATA(iter->chunk), (esize * iter->index));
	FreeNode *ret;
	do {
		ret = curnode;

		if (++iter->index != iter->pool->clength) {
			curnode = POINTER_OFFSET(curnode, esize);
		}
		else {
			iter->index = 0;
			iter->chunk = iter->chunk->next;
			if (iter->chunk == NULL) {
				void *ret2 = (ret->freeword == FREEWORD) ? NULL : ret;

				return ret2;
			}
			curnode = CHUNK_DATA(iter->chunk);
		}
	} while (ret->freeword == FREEWORD);

	return ret;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Thread-Safe Iterator
 * \{ */

static void mempool_threadsafe_iternew(MemPool *pool, MemPoolThreadSafeIter *ts_iter) {
	LIB_memory_pool_iternew(pool, &ts_iter->iter);
	ts_iter->curchunk_threaded_shared = NULL;
}

ParallelMempoolTaskData *mempool_iter_threadsafe_create(MemPool *pool, size_t iter_num) {
	ROSE_assert(pool->flag & ROSE_MEMPOOL_ALLOW_ITER);

	ParallelMempoolTaskData *iter_arr = MEM_mallocN(sizeof(*iter_arr) * iter_num, __func__);
	PoolChunk **curchunk_threaded_shared = MEM_mallocN(sizeof(void *), __func__);

	mempool_threadsafe_iternew(pool, &iter_arr->ts_iter);

	*curchunk_threaded_shared = iter_arr->ts_iter.iter.chunk;
	iter_arr->ts_iter.curchunk_threaded_shared = curchunk_threaded_shared;
	for (size_t i = 1; i < iter_num; i++) {
		iter_arr[i].ts_iter = iter_arr[0].ts_iter;
		*curchunk_threaded_shared = iter_arr[i].ts_iter.iter.chunk = ((*curchunk_threaded_shared) ? (*curchunk_threaded_shared)->next : NULL);
	}

	return iter_arr;
}

void mempool_iter_threadsafe_destroy(ParallelMempoolTaskData *iter_arr) {
	ROSE_assert(iter_arr->ts_iter.curchunk_threaded_shared != NULL);

	MEM_freeN(iter_arr->ts_iter.curchunk_threaded_shared);
	MEM_freeN(iter_arr);
}

void *mempool_iter_threadsafe_step(MemPoolThreadSafeIter *ts_iter) {
	MemPoolIter *iter = &ts_iter->iter;
	if (iter->chunk == NULL) {
		return NULL;
	}

	const uint esize = iter->pool->esize;
	FreeNode *curnode = POINTER_OFFSET(CHUNK_DATA(iter->chunk), (esize * iter->index));
	FreeNode *ret;
	do {
		ret = curnode;

		if (++iter->index != iter->pool->clength) {
			curnode = POINTER_OFFSET(curnode, esize);
		}
		else {
			iter->index = 0;

			/* Begin unique to the `threadsafe` version of this function. */
			for (iter->chunk = *ts_iter->curchunk_threaded_shared; (iter->chunk != NULL) && (atomic_cas_ptr((void **)ts_iter->curchunk_threaded_shared, iter->chunk, iter->chunk->next) != iter->chunk); iter->chunk = *ts_iter->curchunk_threaded_shared) {
				/* pass. */
			}
			if (iter->chunk == NULL) {
				return (ret->freeword == FREEWORD) ? NULL : ret;
			}
			/* End `threadsafe` exception. */

			iter->chunk = iter->chunk->next;
			if (iter->chunk == NULL) {
				return (ret->freeword == FREEWORD) ? NULL : ret;
			}
			curnode = CHUNK_DATA(iter->chunk);
		}
	} while (ret->freeword == FREEWORD);

	return ret;
}

/** \} */
