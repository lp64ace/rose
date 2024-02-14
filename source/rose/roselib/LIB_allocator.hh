#pragma once

#include <algorithm>
#include <stdlib.h>

#include "MEM_alloc.h"

#include "LIB_utildefines.h"

namespace rose {

class GuardedAllocator {
public:
	void *allocate(const size_t size, const size_t alignment, const char *name) {
		return MEM_mallocN_aligned(size, alignment, name);
	}
	
	void deallocate(void *ptr) {
		return MEM_freeN(ptr);
	}
};

}  // namespace rose
