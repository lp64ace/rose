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
typedef struct SpaceLink SpacePropeties;

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Space
 * \{ */

#define FILE_MAXDIR 768
#define FILE_MAXFILE 256
#define FILE_MAX 1024

typedef struct FileSelectParams {
	/** Title, also used for the text of the execute button. */
	char title[64];

	char dir[FILE_MAXDIR];
	char file[FILE_MAXFILE];

	int flag;
} FileSelectParams;

/** #FileSelectParams->flag */
enum {
	FILE_DIRSEL_ONLY = (1 << 0),
	FILE_CHECK_EXISTING = (1 << 1),
};

typedef struct SpaceFile {
	struct SpaceLink *prev, *next;

	int spacetype;
	int flag;

	ListBase regionbase;
	/* End 'SpaceLink' header. */

	FileSelectParams *params;

	/**
	 * The operator that is invoking file-select `op->exec()` will be called on the 'Load' button.
	 * if operator provides op->cancel(), then this will be invoked on the cancel button.
	 */
	struct wmOperator *op;
} SpaceFile;

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
/** \name Space Defines
 * \{ */

enum {
	SPACE_EMPTY = 0,
	SPACE_STATUSBAR,
	SPACE_TOPBAR,
	SPACE_VIEW3D,
	SPACE_PROPERTIES,
	SPACE_FILE,
};

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DNA_SPACE_TYPES_H
