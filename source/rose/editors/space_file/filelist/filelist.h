#ifndef SPACE_FILE_FILELIST_H
#define SPACE_FILE_FILELIST_H

#include "DNA_space_types.h"
#include "DNA_windowmanager_types.h"

#include <stdbool.h>

#define FILEDIR_NBR_ENTRIES_UNSET -1

struct FileList;
struct FileListReadJob;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FileList {
	FileDirEntryArr filelist;
	FileDirEntryArr filelist_intern;

	int tags;
	int type;
	int flags;
	
	/**
	 * Set given path as root directory.
	 *
	 * \param do_change: When true, the callback may change given string in place to a valid value.
	 * \return True when `dirpath` is valid.
	 */
	bool (*check_dir_fn)(const struct FileList *filelist, char dirpath[FILE_MAX], const bool do_change);

	/** Fill `filelist` (to be called by read job). */
	void (*read_job_fn)(struct FileListReadJob *job_params, bool *stop, bool *do_update, float *progress);
} FileList;

/** #FileList->tags */
enum {
	FILELIST_TAGS_NO_THREADS = 1 << 0,
};

/** #FileList->flags */
enum {
	FL_IS_READY = 1 << 0,
	FL_IS_PENDING = 1 << 1,
	FL_IS_SORTED = 1 << 2,

	FL_FORCE_RESET = 1 << 8,
};

struct FileList *filelist_new(int type);
void filelist_settype(struct FileList *filelist, int type);
void filelist_clear(struct FileList *filelist);
void filelist_sort(struct FileList *filelist);
void filelist_free(struct FileList *filelist);

const char *filelist_dir(const struct FileList *filelist);
bool filelist_is_dir(const struct FileList *filelist, const char *path);
/**
 * May modify in place given `dirpath`, which is expected to be #FILE_MAX length.
 */
void filelist_setdir(struct FileList *filelist, char dirpath[FILE_MAX]);

struct FileDirEntry *filelist_file(struct FileList *filelist, size_t index);
void filelist_file_set(struct rContext *C, struct SpaceFile *sfile, size_t index);

/**
 * Limited version of full update done by space_file's file_refresh(),
 * to be used by operators and such.
 * Ensures given filelist is ready to be used (i.e. it is filtered and sorted),
 * unless it is tagged for a full refresh.
 */
size_t filelist_files_ensure(struct FileList *filelist);
size_t filelist_needs_reading(const struct FileList *filelist);

bool filelist_needs_force_reset(const struct FileList *filelist);
void filelist_tag_force_reset(struct FileList *filelist);

bool filelist_ready(const struct FileList *filelist);
bool filelist_pending(const struct FileList *filelist);

void filelist_readjob_start(struct FileList *filelist, const struct rContext *C);

#ifdef __cplusplus
}
#endif

#endif	// !SPACE_FILE_FILELIST_H
