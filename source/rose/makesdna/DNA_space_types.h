#pragma once

#include "DNA_listbase.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name SpaceLink (Base)
 * \{ */

typedef struct SpaceLink {
	struct SpaceLink *prev, *next;

	ListBase regionbase;

	int spacetype;
	int flag;
} SpaceLink;

DNA_ACTION_DEFINE(SpaceLink, DNA_ACTION_STORE);

enum {
	/**
	 * The space is not a regular one opened through the editor menu (for example)
	 * but spawned by an opertor to fulfill some task and then disappear again.
	 */
	SPACE_FLAG_TYPE_TEMPORARY = (1 << 0),
};

/* \} */

/* -------------------------------------------------------------------- */
/** \name Top Bar
 * \{ */

typedef struct SpaceTopBar {
	struct SpaceLink *prev, *next;

	ListBase regionbase;

	int spacetype;
	int flag;
	/* End 'SpaceLink' header. */

	int drag[2];
} SpaceTopBar;

DNA_ACTION_DEFINE(SpaceTopBar, DNA_ACTION_STORE);

/** \} */

/* -------------------------------------------------------------------- */
/** \name View3D
 * \{ */

typedef struct SpaceView3D {
	struct SpaceLink *prev, *next;

	ListBase regionbase;

	int spacetype;
	int flag;
	/* End 'SpaceLink' header. */
	
	float last_draw_time;
} SpaceView3D;

DNA_ACTION_DEFINE(SpaceView3D, DNA_ACTION_STORE);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Status Bar
 * \{ */

typedef struct SpaceStatusBar {
	struct SpaceLink *prev, *next;

	ListBase regionbase;

	int spacetype;
	int flag;
	/* End 'SpaceLink' header. */
} SpaceStatusBar;

DNA_ACTION_DEFINE(SpaceStatusBar, DNA_ACTION_STORE);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Space Definess
 * \{ */

enum {
	SPACE_EMPTY = 0,
	SPACE_TOPBAR = 1,
	SPACE_STATUSBAR = 2,
	SPACE_VIEW3D = 3,
};

#define SPACE_TYPE_NUM (SPACE_VIEW3D + 1)
#define SPACE_TYPE_ANY -1

/* \} */

#ifdef __cplusplus
}
#endif
