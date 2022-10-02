#include "lib/lib_alloc.h"

#include <malloc.h>
#include <stdio.h>

struct MemHead {
#ifdef _DEBUG
	char Name [ 64 ];
#endif
	size_t Size;
};

#define MEMHEAD_FROM_PTR(ptr)		((ptr)?(((MemHead *)ptr)-1):((MemHead *)ptr))
#define PTR_FROM_MEMHEAD(memhead)	((memhead)?(((MemHead *)memhead)+1):(memhead))

void *MEM_mallocN ( size_t length , const char *name ) {
	MemHead *head = ( MemHead * ) malloc ( sizeof ( MemHead ) + length );
	if ( !head ) {
		return nullptr;
	}
	head->Size = length;
#ifdef _DEBUG
	strcpy_s ( head->Name , sizeof ( head->Name ) , name );
#endif
	return PTR_FROM_MEMHEAD ( head );
}

void *MEM_callocN ( size_t length , const char *name ) {
	MemHead *head = ( MemHead * ) calloc ( sizeof ( MemHead ) + length , 1 );
	if ( !head ) {
		return nullptr;
	}
	head->Size = length;
#ifdef _DEBUG
	strcpy_s ( head->Name , sizeof ( head->Name ) , name );
#endif
	return PTR_FROM_MEMHEAD ( head );
}

void *MEM_reallocN ( void *_memory , size_t length ) {
	MemHead *head = ( MemHead * ) realloc ( MEMHEAD_FROM_PTR ( _memory ) , length + sizeof ( MemHead ) );
	if ( !head ) {
		return nullptr;
	}
	head->Size = length;
	return PTR_FROM_MEMHEAD ( head );
}

void *MEM_recallocN ( void *_memory , size_t length ) {
	MemHead *head = ( MemHead * ) realloc ( MEMHEAD_FROM_PTR ( _memory ) , length + sizeof ( MemHead ) );
	if ( !head ) {
		return nullptr;
	}
	head->Size = length;
	memset ( PTR_FROM_MEMHEAD ( head ) , 0 , length );
	return PTR_FROM_MEMHEAD ( head );
}

void *MEM_dupallocN ( void *ptr ) {
	MemHead *head = ( MemHead * ) malloc ( sizeof ( MemHead ) + MEMHEAD_FROM_PTR ( ptr )->Size );
	if ( !head ) {
		return nullptr;
	}
	memcpy ( head , MEMHEAD_FROM_PTR ( ptr ) , sizeof ( MemHead ) + MEMHEAD_FROM_PTR ( ptr )->Size );
	return PTR_FROM_MEMHEAD ( head );
}

void MEM_freeN ( void *ptr ) {
	return free ( MEMHEAD_FROM_PTR ( ptr ) );
}
