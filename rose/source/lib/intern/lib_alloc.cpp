#include "lib/lib_alloc.h"

#include <malloc.h>
#include <stdio.h>

struct MemHead {
	size_t Size;
};

static_assert ( MEM_SIZE_OVERHEAD >= sizeof ( MemHead ) , "MemHead size is too big" );
#ifdef _DEBUG
static_assert ( MEM_SIZE_OVERHEAD == sizeof ( MemHead ) , "MemHead size has changed" );
#endif

#define MEMHEAD_FROM_PTR(ptr)		((ptr)?(((MemHead *)ptr)-1):((MemHead *)ptr))
#define PTR_FROM_MEMHEAD(memhead)	((memhead)?(((MemHead *)memhead)+1):(memhead))

void *MEM_mallocN ( size_t length , const char *name ) {
	MemHead *head = ( MemHead * ) malloc ( MEM_SIZE_OVERHEAD + length );
	if ( !head ) {
		return nullptr;
	}
	head->Size = length;
	return PTR_FROM_MEMHEAD ( head );
}

void *MEM_callocN ( size_t length , const char *name ) {
	MemHead *head = ( MemHead * ) calloc ( MEM_SIZE_OVERHEAD + length , 1 );
	if ( !head ) {
		return nullptr;
	}
	head->Size = length;
	return PTR_FROM_MEMHEAD ( head );
}

void *MEM_malloc_arrayN ( size_t element , size_t length , const char *name ) {
	return MEM_mallocN ( element * length , name );
}

void *MEM_calloc_arrayN ( size_t element , size_t length , const char *name ) {
	return MEM_callocN ( element * length , name );
}

void *MEM_reallocN ( void *_memory , size_t length ) {
	MemHead *head = ( MemHead * ) realloc ( MEMHEAD_FROM_PTR ( _memory ) , length + MEM_SIZE_OVERHEAD );
	if ( !head ) {
		return nullptr;
	}
	head->Size = length;
	return PTR_FROM_MEMHEAD ( head );
}

void *MEM_recallocN ( void *_memory , size_t length ) {
	MemHead *head = ( MemHead * ) realloc ( MEMHEAD_FROM_PTR ( _memory ) , length + MEM_SIZE_OVERHEAD );
	if ( !head ) {
		return nullptr;
	}
	if ( head->Size < length ) {
		memset ( ( ( unsigned char * ) PTR_FROM_MEMHEAD ( head ) ) + head->Size , 0 , length - head->Size );
	}
	head->Size = length;
	return PTR_FROM_MEMHEAD ( head );
}

void *MEM_dupallocN ( void *ptr ) {
	MemHead *head = ( MemHead * ) malloc ( MEM_SIZE_OVERHEAD + MEMHEAD_FROM_PTR ( ptr )->Size );
	if ( !head ) {
		return nullptr;
	}
	memcpy ( head , MEMHEAD_FROM_PTR ( ptr ) , MEM_SIZE_OVERHEAD + MEMHEAD_FROM_PTR ( ptr )->Size );
	return PTR_FROM_MEMHEAD ( head );
}

void MEM_freeN ( void *ptr ) {
	return free ( MEMHEAD_FROM_PTR ( ptr ) );
}

size_t MEM_allocN_len ( void *memory ) {
	return MEMHEAD_FROM_PTR ( memory )->Size;
}
