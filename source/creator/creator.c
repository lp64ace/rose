#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_system.h"

#include "intern/platform.h"

int main(void) {
	int result = 0;
	
	MEM_use_memleak_detection(true);
	MEM_enable_fail_on_memleak();
	MEM_init_memleak_detection();
	
	LIB_system_signal_callbacks_init();

	if((result = InitPlatform()) == 0) {
		ClosePlatform();
	}

	return result;
}
