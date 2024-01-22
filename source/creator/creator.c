#include "MEM_alloc.h"

int main(void) {
	MEM_use_memleak_detection(true);
	MEM_enable_fail_on_memleak();
	MEM_init_memleak_detection();
	return 0;
}
