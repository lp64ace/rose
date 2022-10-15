#pragma once

#include "lib_alloc.h"
#include <stdio.h>

class Allocator {
public:
	virtual void *Allocate ( size_t size ) {
		return MEM_mallocN ( size , __FUNCTION__ );
	}

	virtual void *Reallocate ( void *buffer , size_t size ) {
		return MEM_reallocN ( buffer , size );
	}

	virtual void Deallocate ( void *buffer ) {
		MEM_freeN ( buffer );
	}
};
