#ifndef LIB_MEMPOOL_H
#define LIB_MEMPOOL_H

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
MemPool *LIB_memory_pool_create(size_t element, size_t perchunk, size_t reserve);

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

#ifdef __cplusplus
}
#endif

#endif	// LIB_MEMPOOL_H
