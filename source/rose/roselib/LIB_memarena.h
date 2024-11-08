#ifndef LIB_MEMARENA_H
#define LIB_MEMARENA_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/**
 * A structure that allows for the allocation of variable-sized memory blocks.
 * Unlike MemPool, which is for fixed-size memory allocations, MemArena manages
 * dynamic memory requests and is optimized for mass allocations with minimal
 * deletions. Ideal for scenarios where memory is allocated frequently,
 * but deallocated rarely or all at once.
 */
typedef struct MemArena MemArena;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Arena Creation
 * \{ */

/**
 * Creates a new memory arena for efficient memory management.
 *
 * Each memory block requested from this arena will come from a larger contiguous
 * memory buffer, which is allocated whenever the arena runs out of available memory.
 * The size of this buffer is determined by the \p size parameter.
 * Memory arenas are particularly useful in scenarios involving bulk allocations
 * of memory with little to no need for individual deletions, as deleting memory
 * blocks individually would be inefficient.
 *
 * \param size The size in bytes of each memory buffer to be allocated for the arena.
 * \param name A name for debugging or memory profiling purposes.
 * \return A pointer to the newly created memory arena.
 */
MemArena *LIB_memory_arena_create(size_t size, const char *name);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Arena Destruction
 * \{ */

/**
 * Destroy the specified memory arena and all associated memory.
 *
 * This function deallocates the memory arena itself, as well as any memory blocks
 * that were previously allocated from it. After calling this function, the arena
 * pointer will no longer be valid, and any memory obtained from the arena will
 * also be freed.
 *
 * \param arena A pointer to the memory arena that is to be destroyed.
 */
void LIB_memory_arena_destroy(MemArena *arena);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Memory Allocation and Deallocation
 * \{ */

/**
 * Allocate memory.
 *
 * \param arena A pointer to the memory arena to allocate from.
 * \param size The size of the memory block to allocate.
 * \return A pointer to the allocated memory block.
 */
void *LIB_memory_arena_malloc(MemArena *arena, size_t size);
/**
 * Allocate zero-initialized memory.
 *
 * \param arena A pointer to the memory arena to allocate from.
 * \param size The size of the memory block to allocate.
 * \return A pointer to the allocated memory block.
 */
void *LIB_memory_arena_calloc(MemArena *arena, size_t size);

/**
 * Free all the memory blocks that were allocated from the specified arena.
 *
 * \param arena A pointer to the memory arena we want to clrear.
 * \note This will not free the memory arena itself.
 */
void LIB_memory_arena_clear(MemArena *arena);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_MEMARENA_H
