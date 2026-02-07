#ifndef NDEBUG
#	include "MEM_guardedalloc.h"
#endif

#include "KER_context.h"
#include "KER_modifier.h"

#include "DEG_depsgraph.h"

#include "WM_api.h"

int main(void) {
#ifndef NDEBUG
	MEM_init_memleak_detection();
	MEM_enable_fail_on_memleak();
	MEM_use_guarded_allocator();
#endif

	rContext *C = CTX_new();

	KER_modifier_init();
	DEG_register_node_types();

	WM_init(C);
	do {
		WM_main(C);
	} while (false);

	return 0;
}
