#ifndef WM_WINDOW_H
#define WM_WINDOW_H

#include "KER_context.h"

#include "DNA_screen_types.h"
#include "DNA_vector_types.h"
#include "DNA_windowmanager_types.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Screen;
struct wmWindow;

/* -------------------------------------------------------------------- */
/** \name Create/Destroy Methods
 * \{ */

struct wmWindow *WM_window_open(struct rContext *C, const char *name, int space_type, bool temp);
/** Closes and completely destroys the window and any resources associated with the window. */
void WM_window_close(struct rContext *C, struct wmWindow *window, bool do_free);
void WM_window_free(struct WindowManager *wm, struct wmWindow *window);

void WM_window_post_quit_event(struct wmWindow *window);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Window Screen
 * \{ */

/** Set the screen of the window for UI rendering. */
void WM_window_set_active_screen(struct rContext *C, struct wmWindow *window, struct Screen *screen);
/** Get the screen of the window for UI rendering. */
struct Screen *WM_window_get_active_screen(const struct wmWindow *window);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Window Cursor
 * \{ */

typedef struct RCursor {
	char *bitmap;
	char *mask;
	char hotx;
	char hoty;
} RCursor;

enum {
	WM_CURSOR_NONE,
	WM_CURSOR_DEFAULT,
	WM_CURSOR_WAIT,
	WM_CURSOR_HELP,
	WM_CURSOR_TEXT,
	WM_CURSOR_CROSS,
	WM_CURSOR_EDIT,
	WM_CURSOR_H_SPLIT,
	WM_CURSOR_V_SPLIT,
	WM_CURSOR_N_ARROW,
	WM_CURSOR_E_ARROW,
	WM_CURSOR_W_ARROW,
	WM_CURSOR_S_ARROW,
	WM_CURSOR_NS_ARROW,
	WM_CURSOR_EW_ARROW,
	WM_CURSOR_X_MOVE,
	WM_CURSOR_Y_MOVE,
	WM_CURSOR_STOP,
	WM_CURSOR_SWAP_AREA,
	WM_CURSOR_NUM,
};

void WM_init_cursor_data(void);
void WM_cursor_set(struct wmWindow *window, int cursor);
bool WM_cursor_set_from_tool(struct wmWindow *window, struct ScrArea *area, struct ARegion *region);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Query Methods
 * \{ */

int WM_window_size_x(const struct wmWindow *window);
int WM_window_size_y(const struct wmWindow *window);
void WM_window_rect_calc(const struct wmWindow *window, rcti *r_rect);
void WM_window_screen_rect_calc(const struct wmWindow *window, rcti *r_rect);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Window Scene
 * \{ */

void WM_window_ensure_active_view_layer(struct wmWindow *window);

/** Get the scene of the window for 3D view rendering. */
struct Scene *WM_window_get_active_scene(struct wmWindow *window);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// WM_WINDOW_H
