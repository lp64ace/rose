#include <stdbool.h>
#include <stdio.h>

#include "MEM_alloc.h"

#include "DNA_screen_types.h"
#include "DNA_windowmanager_types.h"
#include "DNA_workspace_types.h"

#include "KER_context.h"
#include "KER_main.h"
#include "KER_screen.h"
#include "KER_workspace.h"

#include "LIB_listbase.h"
#include "LIB_math.h"
#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "GPU_context.h"
#include "GPU_framebuffer.h"

#include "WM_api.h"
#include "WM_draw.h"
#include "WM_handlers.h"
#include "WM_init_exit.h"
#include "WM_window.h"

bool WM_event_drag_test(const wmEvent *event, const int prev_xy[2]) {
	/** Not implemented yet. */
	return false;
}