#ifndef WM_API_H
#define WM_API_H

#include "DNA_windowmanager_types.h"

#include "LIB_utildefines.h"

#include "WM_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

struct rContext;

/* -------------------------------------------------------------------- */
/** \name Main Loop
 * \{ */

void WM_init(struct rContext *C);
void WM_main(struct rContext *C);
void WM_exit(struct rContext *C);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Projection
 * \{ */

void WM_get_projection_matrix(float r_mat[4][4], const struct rcti *rect);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Clipboard
 * \{ */

char *WM_clipboard_text_get_firstline(struct rContext *C, bool selection, unsigned int *r_len);
void WM_clipboard_text_set(struct rContext *C, const char *buf, bool selection);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Clipboard
 * \{ */

float WM_time(struct rContext *C);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// WM_API_H
