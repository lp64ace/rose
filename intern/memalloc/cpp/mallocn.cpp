#include "MEM_alloc.h"
#include <new>

/**
 * These function override the new/delete functions of C++, as a result they are used when `std::shared_ptr<Global>()` and
 * `std::shared_ptr<Local>()` is used to create the memory usage statistics. And since they outlive the `MemLeakPrinter` class
 * they appear as non-freed blocks, causing to log that 3 memory blocks are deleted after the MemLeak has run or that there 
 * are still allocated blocks when the MemLeak is run.
 *
 * The MemBase for these blocks should look like this;
 * MemBase = {
 *   [name = C++/anonymous], length = 152, pointer = ...],
 *   [name = C++/anonymous], length = 16, pointer = ...],
 *   [name = C++/anonymous], length = 8, pointer = ...],
 *   ... // possibly more if more threads allocate memory!
 * };
 *
 * So when we use fail on memleak just ignore this file. If someone can fix this please do.
 */

void *operator new(size_t size, const char *str);
void *operator new[](size_t size, const char *str);

void *operator new(size_t size, const char *str) {
	return MEM_mallocN(size, str);
}
void *operator new[](size_t size, const char *str) {
	return MEM_mallocN(size, str);
}

void *operator new(size_t size) {
	return MEM_mallocN(size, "C++/anonymous");
}
void *operator new[](size_t size) {
	return MEM_mallocN(size, "C++/anonymous[]");
}

void operator delete(void *p) throw() {
	if (p) {
		MEM_freeN(p);
	}
}
void operator delete[](void *p) throw() {
	if (p) {
		MEM_freeN(p);
	}
}
