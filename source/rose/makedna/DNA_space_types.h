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

	unsigned int local_collections_uuid;
} View3D;

/** #View3D->flag */
enum {
	V3D_LOCAL_COLLECTIONS = 1 << 0,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name UserPref Space
 * \{ */

typedef struct SpaceUser {
	struct SpaceLink *prev, *next;

	int spacetype;
	int flag;

	ListBase regionbase;
	/** End of 'SpaceLink' header. */
} SpaceUser;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Test Space
 * \{ */

 typedef struct SpaceTest {
	struct SpaceLink *prev, *next;

	int spacetype;
	int flag;

	ListBase regionbase;
	/** End of 'SpaceLink' header. */
} SpaceTest;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Space Defines
 * \{ */

enum {
	SPACE_EMPTY = 0,
	SPACE_STATUSBAR,
	SPACE_TOPBAR,
	SPACE_VIEW3D,
	SPACE_USERPREF,

	SPACE_TEST = 0xff,
};

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DNA_SPACE_TYPES_H
