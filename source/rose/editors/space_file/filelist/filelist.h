#ifndef SPACE_FILE_FILELIST_H
#define SPACE_FILE_FILELIST_H

#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include <stdbool.h>

struct FileList;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FileList {
	FileDirEntryArr filelist;

	int type;
	
	/**
	 * Set given path as root directory.
	 *
	 * \param do_change: When true, the callback may change given string in place to a valid value.
	 * \return True when `dirpath` is valid.
	 */
	bool (*check_dir_fn)(const struct FileList *filelist, char dirpath[FILE_MAX], const bool do_change);
} FileList;

struct FileList *filelist_new(int type);
void filelist_settype(struct FileList *filelist, int type);
void filelist_clear(struct FileList *filelist);
void filelist_free(struct FileList *filelist);

const char *filelist_dir(const struct FileList *filelist);
bool filelist_is_dir(const struct FileList *filelist, const char *path);
/**
 * May modify in place given `dirpath`, which is expected to be #FILE_MAX length.
 */
void filelist_setdir(struct FileList *filelist, char dirpath[FILE_MAX]);

#ifdef __cplusplus
}
#endif

#endif	// !SPACE_FILE_FILELIST_H
