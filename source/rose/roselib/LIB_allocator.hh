#ifndef LIB_ALLOCATOR_HH
#define LIB_ALLOCATOR_HH

#include "MEM_guardedalloc.h"

#include "malloc.h"

#include "LIB_math_base.h"
#include "LIB_math_bit.h"
#include "LIB_utildefines.h"

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

class RawAllocator {
private:
	struct MemHead {
		int offset;
	};

public:
	void *allocate(size_t size, size_t alignment, const char * /*name*/) {
		ROSE_assert(is_power_of_2_i(int(alignment)));
		void *ptr = malloc(size + alignment + sizeof(MemHead));
		void *used_ptr = reinterpret_cast<void *>(uintptr_t(POINTER_OFFSET(ptr, alignment + sizeof(MemHead))) & ~(uintptr_t(alignment) - 1));
		int offset = int(intptr_t(used_ptr) - intptr_t(ptr));
		ROSE_assert(offset >= int(sizeof(MemHead)));
		(static_cast<MemHead *>(used_ptr) - 1)->offset = offset;
		return used_ptr;
	}

	void deallocate(void *ptr) {
		MemHead *head = static_cast<MemHead *>(ptr) - 1;
		int offset = -head->offset;
		void *actual_pointer = POINTER_OFFSET(ptr, offset);
		free(actual_pointer);
	}
};

}  // namespace rose

#endif	// LIB_ALLOCATOR_HH
