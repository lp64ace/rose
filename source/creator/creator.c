#ifndef NDEBUG
#	include "MEM_guardedalloc.h"
#endif

#include "WM_api.h"

int main(void) {
#ifndef NDEBUG
	MEM_init_memleak_detection();
	MEM_enable_fail_on_memleak();
	MEM_use_guarded_allocator();
#endif
	
	WM_init();
	do {
		WM_main();
	} while(true);
	
	return 0;
}
