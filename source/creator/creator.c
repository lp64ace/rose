#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_system.h"
#include "LIB_utildefines.h"

#include "KER_context.h"
#include "KER_cpp_types.h"
#include "KER_idtype.h"
#include "KER_rose.h"

#include "DNA_genfile.h"

#include "WM_init_exit.h"

#include <stdio.h>
#include <stdlib.h>

int main(void) {
	Context *C;

	MEM_use_memleak_detection(true);
	MEM_enable_fail_on_memleak();
	MEM_init_memleak_detection();

	LIB_system_signal_callbacks_init();
	
	DNA_sdna_current_init();

	KER_cpp_types_init();
	KER_rose_globals_init();
	KER_idtype_init();

	C = CTX_new();

	do {
		WM_init(C);
		WM_main(C);
	} while (false);

	return 0;
}
