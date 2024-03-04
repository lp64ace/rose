#include "testing.h"

#include "MEM_alloc.h"

#include "LIB_system.h"

int main(int argc, char **argv) {
	MEM_use_guarded_allocator();
	MEM_init_memleak_detection();
	MEM_enable_fail_on_memleak();
	
	testing::InitGoogleTest(&argc, argv);
	
	return RUN_ALL_TESTS();
}
