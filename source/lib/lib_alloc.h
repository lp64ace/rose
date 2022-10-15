#pragma once

#include <string.h>

#define MEM_SIZE_OVERHEAD	sizeof(size_t)

#ifdef __cplusplus
extern "C" {
#endif

	// Allocate memory
	void *MEM_mallocN ( size_t length , const char *name );

	// Allocate array memory
	void *MEM_malloc_arrayN ( size_t element , size_t length , const char *name );

	// Allocate memory and init to zero
	void *MEM_callocN ( size_t length , const char *name );

	// Allocate array memory and init to zero
	void *MEM_calloc_arrayN ( size_t element , size_t length , const char *name );

	// Reallocate memory
	void *MEM_reallocN ( void *memory , size_t length );

	// Reallocate memory and init to zero
	void *MEM_recallocN ( void *memory , size_t length );

	// Duplicates a block of memory, and returns a pointer to the newly allocated block.
	void *MEM_dupallocN ( void *memory );

	// Deallocate memory
	void MEM_freeN ( void *memory );

	size_t MEM_allocN_len ( void *memory );

#define MEM_SAFE_FREE(mem)		{if(mem){MEM_freeN(mem);(mem)=nullptr;}}

#ifdef __cplusplus
}
#endif
