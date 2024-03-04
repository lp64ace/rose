#include "LIB_assert.h"
#include "LIB_endian.h"
#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include "MEM_alloc.h"

/* clang-format off */

#if defined(BIG_ENDIAN)
#	define MAKE_ID(a, b, c, d) ((int32_t)(a) << 24 | (int32_t)(b) << 16 | (int32_t)(c) << 8 | (int32_t)(d))
#	define MAKE_ID8(a, b, c, d, e, f, g, h) \
	((int64_t)(a) << 56 | (int64_t)(b) << 48 | (int64_t)(c) << 40 | (int64_t)(d) << 32 | \
	 (int64_t)(e) << 24 | (int64_t)(f) << 16 | (int64_t)(g) << 8 | (int64_t)(h))
#elif defined(LITTLE_ENDIAN)
#	define MAKE_ID(a, b, c, d) ((int32_t)(d) << 24 | (int32_t)(c) << 16 | (int32_t)(b) << 8 | (int32_t)(a))
#	define MAKE_ID8(a, b, c, d, e, f, g, h) \
	((int64_t)(h) << 56 | (int64_t)(g) << 48 | (int64_t)(f) << 40 | (int64_t)(e) << 32 | \
	 (int64_t)(d) << 24 | (int64_t)(c) << 16 | (int64_t)(b) << 8 | (int64_t)(a))
#else
#	error "Unkown endian, cannot make system generic word for mempool."
#endif

/* clang-format on */

#define FREEWORD ((sizeof(void *) > sizeof(int32_t)) ? MAKE_ID8('e', 'e', 'r', 'f', 'f', 'r', 'e', 'e') : MAKE_ID('e', 'f', 'f', 'e'))
#define USEDWORD MAKE_ID('u', 's', 'e', 'd')

#define USE_CHUNK_POW2

typedef struct LIB_freenode {
	struct LIB_freenode *next;

	intptr_t freeword;
} LIB_freenode;

typedef struct LIB_mempool_chunk {
	struct LIB_mempool_chunk *next;
} LIB_mempool_chunk;

typedef struct LIB_mempool {
	LIB_mempool_chunk *chunks;
	LIB_mempool_chunk *chunk_tail;

	size_t esize;
	size_t csize;
	size_t pchunk;
	int flag;

	LIB_freenode *free;
	size_t maxchunks;
	size_t totused;
	size_t totalloc;
} LIB_mempool;

#define MEMPOOL_ELEM_SIZE_MIN (sizeof(void *) * 2)

#define CHUNK_DATA(chunk) (void *)((chunk) + 1)

#define NODE_STEP_NEXT(node) ((void *)((char *)(node) + esize))
#define NODE_STEP_PREV(node) ((void *)((char *)(node)-esize))

#define CHUNK_OVERHEAD (uint)(MEM_SIZE_OVERHEAD + sizeof(LIB_mempool_chunk))

#if defined(USE_CHUNK_POW2)
static size_t power_of_2_max_z(size_t x) {
	x -= 1;
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >> 16);
	x = x | (x >> 32);
	return x + 1;
}
#endif

ROSE_INLINE LIB_mempool_chunk *mempool_chunk_find(LIB_mempool_chunk *head, size_t index) {
	while (index-- && head) {
		head = head->next;
	}
	return head;
}

ROSE_INLINE size_t mempool_maxchunks(const size_t elem_num, const size_t pchunk) {
	return (elem_num <= pchunk) ? 1 : ((elem_num / pchunk) + 1);
}

static LIB_mempool_chunk *mempool_chunk_alloc(LIB_mempool *pool) {
	return MEM_mallocN(sizeof(LIB_mempool_chunk) + pool->csize, "rose::MemPool::chunk");
}

static LIB_freenode *mempool_chunk_add(LIB_mempool *pool, LIB_mempool_chunk *chunk, LIB_freenode *last) {
	const size_t esize = pool->esize;
	LIB_freenode *curnode = CHUNK_DATA(chunk);
	size_t j;

	if (pool->chunk_tail) {
		pool->chunk_tail->next = chunk;
	}
	else {
		ROSE_assert(pool->chunks == NULL);
		pool->chunks = chunk;
	}

	chunk->next = NULL;
	pool->chunk_tail = chunk;

	if (pool->free == NULL) {
		pool->free = curnode;
	}

	j = pool->pchunk;
	if (pool->flag & ROSE_MEMPOOL_ALLOW_ITER) {
		while (j--) {
			LIB_freenode *next;

			curnode->next = next = NODE_STEP_NEXT(curnode);
			curnode->freeword = FREEWORD;

			curnode = next;
		}
	}
	else {
		while (j--) {
			LIB_freenode *next;

			curnode->next = next = NODE_STEP_NEXT(curnode);

			curnode = next;
		}
	}

	curnode = NODE_STEP_PREV(curnode);

	curnode->next = NULL;

	pool->totalloc += pool->pchunk;

	if (last) {
		last->next = CHUNK_DATA(chunk);
	}

	return curnode;
}

static void mempool_chunk_free(LIB_mempool_chunk *chunk, LIB_mempool *pool) {
	UNUSED_VARS(pool);

	MEM_freeN(chunk);
}

static void mempool_chunk_free_all(LIB_mempool_chunk *chunk, LIB_mempool *pool) {
	LIB_mempool_chunk *next;

	for (; chunk; chunk = next) {
		next = chunk->next;

		mempool_chunk_free(chunk, pool);
	}
}

LIB_mempool *LIB_mempool_create(size_t esize, size_t elem_num, size_t perchunk, int flag) {
	LIB_mempool *pool;
	LIB_freenode *last_tail = NULL;
	size_t i, maxchunks;

	pool = MEM_mallocN(sizeof(LIB_mempool), "rose::MemoryPool");

	if (esize < MEMPOOL_ELEM_SIZE_MIN) {
		esize = MEMPOOL_ELEM_SIZE_MIN;
	}

	if (flag & ROSE_MEMPOOL_ALLOW_ITER) {
		esize = ROSE_MAX(esize, sizeof(LIB_freenode));
	}

	maxchunks = mempool_maxchunks(elem_num, perchunk);

	pool->chunks = NULL;
	pool->chunk_tail = NULL;
	pool->esize = esize;

#ifdef USE_CHUNK_POW2
	{
		ROSE_assert(power_of_2_max_z(perchunk * esize) > CHUNK_OVERHEAD);
		perchunk = (power_of_2_max_z(perchunk * esize) - CHUNK_OVERHEAD) / esize;
	}
#endif

	pool->csize = esize * perchunk;

	/* Ensure this is a power of 2, minus the rounding by element size. */
#if defined(USE_CHUNK_POW2) && !defined(NDEBUG)
	{
		size_t final_size = MEM_SIZE_OVERHEAD + sizeof(LIB_mempool_chunk) + pool->csize;
		ROSE_assert(((size_t)power_of_2_max_z(final_size) - final_size) < pool->esize);
	}
#endif

	pool->pchunk = perchunk;
	pool->flag = flag;
	pool->free = NULL;
	pool->maxchunks = maxchunks;
	pool->totalloc = 0;
	pool->totused = 0;

	if (elem_num) {
		for (i = 0; i < maxchunks; i++) {
			LIB_mempool_chunk *chunk = mempool_chunk_alloc(pool);
			last_tail = mempool_chunk_add(pool, chunk, last_tail);
		}
	}

	return pool;
}

void *LIB_mempool_alloc(LIB_mempool *pool) {
	LIB_freenode *free_pop;

	if (pool->free == NULL) {
		LIB_mempool_chunk *chunk = mempool_chunk_alloc(pool);
		mempool_chunk_add(pool, chunk, NULL);
	}

	ROSE_assert(pool->free);

	free_pop = pool->free;

	ROSE_assert(pool->chunk_tail->next == NULL);

	if (pool->flag & ROSE_MEMPOOL_ALLOW_ITER) {
		free_pop->freeword = USEDWORD;
	}

	pool->free = free_pop->next;
	pool->totused++;

	return (void *)free_pop;
}

void *LIB_mempool_calloc(LIB_mempool *pool) {
	void *retval = LIB_mempool_alloc(pool);

	memset(retval, 0, pool->esize);

	return retval;
}

void LIB_mempool_free(LIB_mempool *pool, void *ptr) {
	LIB_freenode *newhead = ptr;

#ifndef NDEBUG
	memset(ptr, 255, pool->esize);
#endif

	if (pool->flag & ROSE_MEMPOOL_ALLOW_ITER) {
		/** This assert will detect double frees. */
		ROSE_assert(newhead->freeword != FREEWORD);
		newhead->freeword = FREEWORD;
	}

	newhead->next = pool->free;
	pool->free = newhead;

	pool->totused--;

	if (pool->totused == 0 && pool->chunks->next) {
		const size_t esize = pool->esize;
		size_t j;

		LIB_freenode *curnode;
		LIB_mempool_chunk *first = pool->chunks;

		mempool_chunk_free_all(first->next, pool);

		first->next = NULL;
		pool->chunk_tail = first;

		pool->totalloc = pool->pchunk;

		curnode = CHUNK_DATA(first);
		pool->free = curnode;

		j = pool->pchunk;
		while (j--) {
			LIB_freenode *next = curnode->next = NODE_STEP_NEXT(curnode);
			curnode = next;
		}

		curnode->next = NULL; /* terminate the list */
	}
}

void LIB_mempool_clear_ex(LIB_mempool *pool, size_t reserve) {
	LIB_mempool_chunk *chunk;
	LIB_mempool_chunk *chunk_next;
	size_t maxchunks;

	LIB_mempool_chunk *chunks_temp;
	LIB_freenode *last_tail = NULL;

	if (reserve == 0) {
		maxchunks = pool->maxchunks;
	}
	else {
		maxchunks = mempool_maxchunks(reserve, pool->pchunk);
	}

	chunk = mempool_chunk_find(pool->chunks, maxchunks - 1);
	if (chunk && chunk->next) {
		chunk_next = chunk->next;
		chunk->next = NULL;
		chunk = chunk_next;

		do {
			chunk_next = chunk->next;
			mempool_chunk_free(chunk, pool);
		} while (chunk = chunk_next);
	}

	pool->free = NULL;
	pool->totused = 0;
	pool->totalloc = 0;

	chunks_temp = pool->chunks;
	pool->chunks = NULL;
	pool->chunk_tail = NULL;

	while (chunk = chunks_temp) {
		chunks_temp = chunk->next;
		last_tail = mempool_chunk_add(pool, chunk, last_tail);
	}
}
void LIB_mempool_clear(LIB_mempool *pool) {
	LIB_mempool_clear_ex(pool, 0);
}

size_t LIB_mempool_length(LIB_mempool *pool) {
	return pool->totused;
}

void LIB_mempool_destroy(LIB_mempool *pool) {
	mempool_chunk_free_all(pool->chunks, pool);

	MEM_freeN(pool);
}

void LIB_mempool_iternew(LIB_mempool *pool, LIB_mempool_iter *itr) {
	ROSE_assert(pool->flag & ROSE_MEMPOOL_ALLOW_ITER);

	itr->pool = pool;
	itr->chunk = pool->chunks;
	itr->index = 0;
}
void *LIB_mempool_iterstep(LIB_mempool_iter *itr) {
	if (itr->chunk == NULL) {
		return NULL;
	}

	const size_t esize = itr->pool->esize;
	LIB_freenode *curnode = POINTER_OFFSET(CHUNK_DATA(itr->chunk), (esize * itr->index));
	LIB_freenode *ret;
	do {
		ret = curnode;

		if (++itr->index != itr->pool->pchunk) {
			curnode = POINTER_OFFSET(curnode, esize);
		}
		else {
			itr->index = 0;
			itr->chunk = itr->chunk->next;
			if (UNLIKELY(itr->chunk == NULL)) {
				void *ret2 = (ret->freeword == FREEWORD) ? NULL : ret;

				return ret2;
			}
			curnode = CHUNK_DATA(itr->chunk);
		}
	} while (ret->freeword == FREEWORD);

	return ret;
}
