#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_system.h"

int main(void) {
	LIB_system_signal_callbacks_init();
	
	MEM_use_memleak_detection(true);
	MEM_enable_fail_on_memleak();
	MEM_init_memleak_detection();
	return 0;
}
