#include "ED_screen.h"

#include "KER_screen.h"
#include "KER_workspace.h"

#include "WM_api.h"

#include "screen_intern.h"

/* -------------------------------------------------------------------- */
/** \name WorkSpace
 * \{ */

struct WorkSpaceLayout *ED_workspace_layout_add(struct Main *main, struct WorkSpace *workspace, struct wmWindow *window, const char *name) {
	Screen *screen;
	rcti screen_rect;
	
	WM_window_screen_rect_calc(window, &screen_rect);
	screen = screen_add(main, name, &screen_rect);
	
	return KER_workspace_layout_new(main, workspace, screen, name);
}

/* \} */
