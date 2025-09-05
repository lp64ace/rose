#ifndef DNA_SPACE_TYPES_H
#define DNA_SPACE_TYPES_H

#include "DNA_listbase.h"
#include "DNA_userdef_types.h"

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
typedef struct SpaceLink SpaceDebug;

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

	Theme editing;
} SpaceUser;

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Space
 * \{ */

 typedef struct SpaceFile {
	struct SpaceLink *prev, *next;

	int spacetype;
	int flag;

	ListBase regionbase;
	/** End of 'SpaceLink' header. */
} SpaceFile;

enum eFile_Types {
	FILE_TYPE_ROSE = 1 << 0,
	FILE_TYPE_IMAGE = 1 << 1,
	FILE_TYPE_MOVIE = 1 << 2,
	FILE_TYPE_SOUND = 1 << 3,
	FILE_TYPE_ASSET = 1 << 4,
	FILE_TYPE_FONT = 1 << 5,
	FILE_TYPE_TEXT = 1 << 6,
	FILE_TYPE_FOLDER = 1 << 7,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Space Defines
 * \{ */

enum {
	SPACE_EMPTY = 0,
	SPACE_DEBUG,
	SPACE_STATUSBAR,
	SPACE_TOPBAR,
	SPACE_VIEW3D,
	SPACE_USERPREF,
	SPACE_FILE,
};

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DNA_SPACE_TYPES_H
