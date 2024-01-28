#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_system.h"
#include "LIB_utildefines.h"

#include "glib.h"

int main(void) {
	int result = 0;

	MEM_use_memleak_detection(true);
	MEM_enable_fail_on_memleak();
	MEM_init_memleak_detection();

	LIB_system_signal_callbacks_init();
	
	GWindow *window = GHOST_InitWindow(NULL, 1440, 800);
	while(GHOST_IsWindow(window)) {
		if(GHOST_ActivateWindowDrawingContext(window)) {
			GHOST_SwapWindowBuffers(window);
		}
		if(GHOST_ProcessEvents(false)) {
			/** A window destroy event might destroy our window! */
			GHOST_DispatchEvents();
		}
	}

	return result;
}
