#ifndef LIB_MEMPOOL_H
#define LIB_MEMPOOL_H

#include "LIB_task.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/**
 * A memory pool, also known as a memory allocator or a memory management pool,
 * is a software or hardware structure used to manage dynamic memory allocation
 * in a computer program.
 *
 * - It is a common technique used to efficiently allocate and deallocate memory
 * for data structures and objects during program execution.
 * It is a pre-allocated region of memory that is divided into fixed-size blocks.
 * Memory pools are a form of dynamic memory allocation that offers a number of
 * advantages over traditional methods such as MEM_mallocN and MEM_freeN.
 *
 * - Fixed-size Memory Pool: All allocated blocks are of the same size.
 * These pools are simple and efficient for managing objects of uniform size,
 * It is commonly used in embedded systems and real-time applications.
 */
typedef struct MemPool MemPool;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pool Creation
 * \{ */

enum {
	ROSE_MEMPOOL_NOP = 0,
	ROSE_MEMPOOL_ALLOW_ITER = 1 << 0,
};

/**
 * \brief Create a new memory pool.
 *
 * Initializes a memory pool for efficient allocation and management of fixed-size elements.
 * The pool will allocate memory in chunks, and optionally reserve some space upfront to avoid
 * initial allocations.
 *
 * \param element Size of each element to be stored in the memory pool.
 * \param perchunk Number of elements to allocate per chunk.
 * \param reserve Number of elements to reserve initially.
 *
 * \return Pointer to the newly created memory pool, or NULL if creation fails.
 */
MemPool *LIB_memory_pool_create(size_t element, size_t perchunk, size_t reserve, int flag);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pool Destruction
 * \{ */

/**
 * \brief Destroy the memory pool.
 *
 * Frees all memory associated with the memory pool and destroys the pool instance.
 * After this function is called, the pool can no longer be used.
 *
 * \param pool Pointer to the memory pool to destroy.
 */
void LIB_memory_pool_destroy(MemPool *pool);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Memory Allocation and Deallocation
 * \{ */

/**
 * \brief Allocate memory from the memory pool.
 *
 * Allocates a single uninitialized block of memory from the memory pool.
 * The pool must have been created beforehand. If there are no available
 * blocks, the pool will automatically expand.
 *
 * \param pool The memory pool from which to allocate memory.
 *
 * \return Pointer to the allocated memory, or NULL if allocation fails.
 */
void *LIB_memory_pool_malloc(MemPool *pool);
/**
 * \brief Allocate and zero-initialize memory from the memory pool.
 *
 * Allocates a single block of memory from the memory pool and initializes
 * it to zero. The memory pool will automatically expand if necessary.
 *
 * \param pool The memory pool from which to allocate memory.
 *
 * \return Pointer to the zero-initialized memory, or NULL if allocation fails.
 */
void *LIB_memory_pool_calloc(MemPool *pool);

/**
 * \brief Free a previously allocated block of memory.
 *
 * Frees a block of memory that was previously allocated from the memory pool.
 * The memory is not actually released back to the system, but made available
 * for future allocations within the pool.
 *
 * \param pool The memory pool from which the block was allocated.
 * \param ptr Pointer to the block of memory to free.
 */
void LIB_memory_pool_free(MemPool *pool, void *ptr);

/**
 * \brief Clear the memory pool and optionally reserve space.
 *
 * Clears all allocated blocks in the memory pool, making the memory available
 * for reuse. The function can optionally retain a reserved number of blocks
 * for future allocations to avoid reallocating memory from the system.
 *
 * \param pool The memory pool to clear.
 * \param reserve The number of blocks to retain in the pool after clearing.
 */
void LIB_memory_pool_clear(MemPool *pool, size_t reserve);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Statistics
 * \{ */

/**
 * \brief Get the current number of elements in the memory pool.
 *
 * This function returns the number of allocated elements currently managed
 * by the memory pool. It provides insight into how many elements have been
 * allocated but does not include any elements that have been freed or are
 * reserved.
 *
 * \param pool The memory pool from which to retrieve the element count.
 *
 * \return The current number of allocated elements in the memory pool.
 */
size_t LIB_memory_pool_length(MemPool *pool);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utils
 * \{ */

/**
 * Returns the element at the specified index.
 */
void *LIB_memory_pool_findelem(MemPool *pool, size_t index);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Iterator
 * \{ */

typedef struct MemPoolIter {
	struct MemPool *pool;
	struct PoolChunk *chunk;

	size_t index;
} MemPoolIter;

void LIB_memory_pool_iternew(MemPool *pool, MemPoolIter *iter);
void *LIB_memory_pool_iterstep(MemPoolIter *iter);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Thread-Safe Iterator
 * \{ */

typedef struct MemPoolThreadSafeIter {
	MemPoolIter iter;
	struct PoolChunk **curchunk_threaded_shared;
} MemPoolThreadSafeIter;

typedef struct ParallelMempoolTaskData {
	MemPoolThreadSafeIter ts_iter;
	TaskParallelTLS tls;
} ParallelMempoolTaskData;

/**
 * Initialize an array of mempool iterators, #LIB_MEMPOOL_ALLOW_ITER flag must be set.
 *
 * This is used in threaded code, to generate as much iterators as needed
 * (each task should have its own),
 * such that each iterator goes over its own single chunk,
 * and only getting the next chunk to iterate over has to be
 * protected against concurrency (which can be done in a lock-less way).
 *
 * To be used when creating a task for each single item in the pool is totally overkill.
 *
 * See #LIB_task_parallel_mempool implementation for detailed usage example.
 */
ParallelMempoolTaskData *mempool_iter_threadsafe_create(MemPool *pool, size_t iter_num);

void mempool_iter_threadsafe_destroy(ParallelMempoolTaskData *iter_arr);

/**
 * A version of #LIB_mempool_iterstep that uses
 * #LIB_mempool_threadsafe_iter.curchunk_threaded_shared for threaded iteration support.
 * (threaded section noted in comments).
 */
void *mempool_iter_threadsafe_step(MemPoolThreadSafeIter *iter);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_MEMPOOL_H
