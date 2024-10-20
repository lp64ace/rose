#ifndef NDEBUG
#	include "MEM_guardedalloc.h"
#endif

int main(void) {
#ifndef NDEBUG
	MEM_init_memleak_detection();
	MEM_enable_fail_on_memleak();
	MEM_use_guarded_allocator();
#endif
	
	return 0;
}
