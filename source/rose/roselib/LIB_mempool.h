#pragma once

#include "LIB_sys_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct LIB_mempool;
struct LIB_mempool_chunk;

typedef struct LIB_mempool LIB_mempool;

enum {
	ROSE_MEMPOOL_NOP = 0,
	ROSE_MEMPOOL_ALLOW_ITER = (1 << 0),
};

LIB_mempool *LIB_mempool_create(size_t esize, size_t elem_num, size_t chunk, int flag);

void *LIB_mempool_alloc(LIB_mempool *pool);
void *LIB_mempool_calloc(LIB_mempool *pool);

void* LIB_mempool_findelem(LIB_mempool* pool, size_t index);

void LIB_mempool_free(LIB_mempool *pool, void *ptr);

void LIB_mempool_clear_ex(LIB_mempool *pool, size_t reserve);
void LIB_mempool_clear(LIB_mempool *pool);

size_t LIB_mempool_length(LIB_mempool *pool);

void LIB_mempool_destroy(LIB_mempool *pool);

typedef struct LIB_mempool_iter {
	struct LIB_mempool *pool;
	struct LIB_mempool_chunk *chunk;
	size_t index;
} LIB_mempool_iter;

void LIB_mempool_iternew(LIB_mempool *pool, LIB_mempool_iter *itr);
void *LIB_mempool_iterstep(LIB_mempool_iter *itr);

#if defined(__cplusplus)
}
#endif
