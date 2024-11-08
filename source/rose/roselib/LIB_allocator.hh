#ifndef LIB_ALLOCATOR_HH
#define LIB_ALLOCATOR_HH

#include "MEM_guardedalloc.h"

namespace rose {

class GuardedAllocator {
public:
	void *allocate(size_t size, size_t alignment, const char *name) {
		return MEM_mallocN_aligned(size, alignment, name);
	}

	void deallocate(void *ptr) {
		MEM_freeN(ptr);
	}
};

}  // namespace rose

#endif	// LIB_ALLOCATOR_HH
