#ifndef DNA_SPACE_TYPES_H
#define DNA_SPACE_TYPES_H

#include "DNA_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name SpaceLink (Base)
 * \{ */

typedef struct SpaceLink {
	struct SpaceLink *prev, *next;
	
	int spacetype;
	int flag;

	ListBase regionbase;
} SpaceLink;

typedef struct SpaceLink SpaceTopBar;
typedef struct SpaceLink SpaceStatusBar;

/** \} */

/* -------------------------------------------------------------------- */
/** \name View3D Space
 * \{ */

typedef struct View3D {
	struct SpaceLink *prev, *next;

	int spacetype;
	int flag;

	ListBase regionbase;
	/** End of 'SpaceLink' header. */
} View3D;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Space Defines
 * \{ */

enum {
	SPACE_EMPTY = 0,
	SPACE_STATUSBAR,
	SPACE_TOPBAR,
	SPACE_VIEW3D,
};

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DNA_SPACE_TYPES_H
