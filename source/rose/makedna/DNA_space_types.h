#ifndef DNA_SPACE_TYPES_H
#define DNA_SPACE_TYPES_H

#include "DNA_listbase.h"
#include "DNA_userdef_types.h"

#include <stdint.h>

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

/** #SpaceLink->flag */
enum {
	SPACE_FLAG_TYPE_TEMPORARY = 1 << 0,
	SPACE_FLAG_TYPE_WAS_ACTIVE = 1 << 1,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Space
 * \{ */

#define FILE_MAXDIR 768
#define FILE_MAXFILE 256
#define FILE_MAX 1024

typedef struct FileDirEntry {
	struct FileDirEntry *prev, *next;

	char name[FILE_MAXFILE];
	char relpath[FILE_MAX];

	uint64_t size;
	uint64_t time;

	int type;
	int flag;

	struct FileDirEntry_DrawData { // Needed for #DNA
		char size[16];
		char date[32];
	} draw_data;
} FileDirEntry;

/** #FileDirEntry->flag */
enum {
	FILE_SEL_HIGHLIGHTED = 1 << 0,
	FILE_SEL_SELECTED = 1 << 1,
};

typedef struct FileDirEntryArr {
	ListBase entries;
	int totentries;

	char root[FILE_MAXDIR];
} FileDirEntryArr;

typedef struct FileSelectParams {
	/** Title, also used for the text of the execute button. */
	char title[64];

	/**
	 * Abused by the editor allowing to write both directory and filename, 
	 * therefore we allow up to #FILE_MAX instead of #FILE_MAXDIR!
	 */
	char dir[FILE_MAX];
	char file[FILE_MAXFILE];

	int type;
	int flag;
} FileSelectParams;

/** #FileSelectParams->flag */
enum {
	FILE_DIRSEL_ONLY = (1 << 0),
	FILE_CHECK_EXISTING = (1 << 1),
};

struct FileList;

typedef struct SpaceFile {
	struct SpaceLink *prev, *next;

	int spacetype;
	int flag;

	ListBase regionbase;
	/* End 'SpaceLink' header. */

	struct FileSelectParams *params;
	struct FileList *files;

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
