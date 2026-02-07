#ifndef LIB_MEMBLOCK_H
#define LIB_MEMBLOCK_H

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ROSE_MEM_BLOCK_CHUNK_SIZE (1 << 15) /* 32KiB */

typedef struct MemBlock MemBlock;
typedef void (*MemblockValFreeFP)(void *val);

/** \note This does not define the number of elements per chunk, but rather defines the size in bytes of the chunk. */
struct MemBlock *LIB_memory_block_create_ex(size_t elem_size, size_t chunk_size);
#define LIB_memory_block_create(nelem) LIB_memory_block_create_ex(nelem, ROSE_MEM_BLOCK_CHUNK_SIZE)

/**
 * Allocate a new memory block in the memory block and return a pointer to it.
 */
void *LIB_memory_block_alloc(MemBlock *block);

/**
 * Reset elem count to 0 but keep as much memory allocated needed
 * for at least the previous elem count.
 */
void LIB_memory_block_clear(MemBlock *block, MemblockValFreeFP free_callback);
void LIB_memory_block_destroy(MemBlock *block, MemblockValFreeFP free_callback);


/**
 * Direct access. elem is element index inside the chosen chunk.
 * Double usage: You can set chunk to 0 and set the absolute elem index.
 * The correct chunk will be retrieve.
 */
void *LIB_memory_block_elem_get(MemBlock *block, size_t chunk, size_t elem);

typedef struct MemBlockIter {
	void **chunk_list;
	size_t current_index;
	size_t end_index;
	size_t chunk_max_offset;
	size_t chunk_idx;
	size_t element_size;
	size_t element_offset;
} MemBlockIter;

void LIB_memory_block_iternew(MemBlock *mblk, MemBlockIter *iter);
void *LIB_memory_block_iterstep(MemBlockIter *iter);

#ifdef __cplusplus
}
#endif

#endif // LIB_MEMBLOCK_H
