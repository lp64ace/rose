#include "MEM_guardedalloc.h"

#include "LIB_memblock.h"

#define CHUNK_LIST_SIZE 16

typedef struct MemBlock {
	void **chunks;

	size_t elem_size;
	size_t elem_next;
	size_t elem_last;
	size_t elem_next_offset;
	size_t chunk_max_offset;
	size_t chunk_next;
	size_t chunk_size;
	size_t chunk_length;
} MemBlock;

MemBlock *LIB_memory_block_create_ex(size_t elem_size, size_t chunk_size) {
	MemBlock *block = MEM_mallocN(sizeof(MemBlock), "MemBlock");

	block->elem_size = elem_size;
	block->elem_next = 0;
	block->elem_last = -1;
	block->chunk_size = chunk_size;
	block->chunk_length = CHUNK_LIST_SIZE;
	block->chunks = MEM_callocN(sizeof(void *) * block->chunk_length, "MemBlock Chunks");
	block->chunks[0] = MEM_callocN_aligned(block->chunk_size, 32, __func__);
	block->chunk_max_offset = (block->chunk_size / block->elem_size) * block->elem_size;
	block->elem_next_offset = 0;
	block->chunk_next = 0;

	return block;
}

void *LIB_memory_block_alloc(MemBlock *block) {
	if (block->elem_last < block->elem_next) {
		block->elem_last = block->elem_next;
	}
	block->elem_next++;

	void *ptr = POINTER_OFFSET(block->chunks[block->chunk_next], block->elem_next_offset);

	block->elem_next_offset += block->elem_size;

	if (block->elem_next_offset == block->chunk_max_offset) {
		block->elem_next_offset = 0;
		block->chunk_next++;

		if (block->chunk_next >= block->chunk_length) {
			block->chunk_length += CHUNK_LIST_SIZE;
			block->chunks = MEM_recallocN(block->chunks, sizeof(void *) * block->chunk_length);
		}

		if (block->chunks[block->chunk_next] == NULL) {
			block->chunks[block->chunk_next] = MEM_callocN_aligned(block->chunk_size, 32, __func__);
		}
	}
	return ptr;
}

void LIB_memory_block_clear(MemBlock *block, MemblockValFreeFP free_callback) {
	size_t elem_per_chunk = block->chunk_size / block->elem_size;
	size_t last_used_chunk = block->elem_next / elem_per_chunk;

	if (free_callback) {
		for (int i = block->elem_last; i >= block->elem_next; i--) {
			int chunk_idx = i / elem_per_chunk;
			int elem_idx = i - elem_per_chunk * chunk_idx;
			void *val = (char *)(block->chunks[chunk_idx]) + block->elem_size * elem_idx;
			free_callback(val);
		}
	}

	for (int i = last_used_chunk + 1; i < block->chunk_length; i++) {
		MEM_SAFE_FREE(block->chunks[i]);
	}

	if (last_used_chunk + 1 < block->chunk_length - CHUNK_LIST_SIZE) {
		block->chunk_length -= CHUNK_LIST_SIZE;
		block->chunks = MEM_recallocN(block->chunks, sizeof(void *) * block->chunk_length);
	}

	block->elem_last = block->elem_next - 1;
	block->elem_next = 0;
	block->elem_next_offset = 0;
	block->chunk_next = 0;
}

void LIB_memory_block_destroy(MemBlock *block, MemblockValFreeFP free_callback) {
	int elem_per_chunk = block->chunk_size / block->elem_size;

	if (free_callback) {
		for (int i = 0; i <= block->elem_last; i++) {
			int chunk_idx = i / elem_per_chunk;
			int elem_idx = i - elem_per_chunk * chunk_idx;
			void *val = (char *)(block->chunks[chunk_idx]) + block->elem_size * elem_idx;
			free_callback(val);
		}
	}

	for (int i = 0; i < block->chunk_length; i++) {
		MEM_SAFE_FREE(block->chunks[i]);
	}
	MEM_SAFE_FREE(block->chunks);
	MEM_freeN(block);
}

void *LIB_memory_block_elem_get(MemBlock *block, size_t chunk, size_t elem) {
	size_t elem_per_chunk = block->chunk_size / block->elem_size;
	chunk += elem / elem_per_chunk;
	elem = elem % elem_per_chunk;
	return POINTER_OFFSET(block->chunks[chunk], block->elem_size * elem);
}
