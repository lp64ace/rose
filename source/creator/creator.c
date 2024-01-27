#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_system.h"

#include "glib.h"

#include <stdio.h>

int main(void) {
	int result = 0;
	
	MEM_use_memleak_detection(true);
	MEM_enable_fail_on_memleak();
	MEM_init_memleak_detection();
	
	LIB_system_signal_callbacks_init();

	GWindow *window = InitWindow(NULL, 1440, 800);
	SetWindowPosition(window, 200, 200);
	
	while(IsWindow(window)) {
		if(ActivateWindowDrawingContext(window)) {
			SwapWindowDrawingBuffers(window);
		}
		
		if(ProcessEvents(false)) {
			DispatchEvents();
		}
	}

	return result;
}
