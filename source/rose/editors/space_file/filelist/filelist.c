#include "MEM_guardedalloc.h"

#include "KER_context.h"
#include "KER_main.h"

#include "LIB_thread.h"
#include "LIB_fileops.h"
#include "LIB_listbase.h"
#include "LIB_sort_utils.h"
#include "LIB_path_utils.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "WM_api.h"

#include "filelist.h"
#include "file.h"

typedef struct FileListReadJob {
	char filepath[FILE_MAX];
	Main *main;
	FileList *filelist;

	SpinLock lock;

	/** Shallow copy of #filelist for thread-safe access.
	 *
	 * The job system calls #filelist_readjob_update which moves any read file from #tmp_filelist
	 * into #filelist in a thread-safe way.
	 *
	 * #tmp_filelist also keeps an `AssetLibrary *` so that it can be loaded in the same thread,
	 * and moved to #filelist once all categories are loaded.
	 *
	 * NOTE: #tmp_filelist is freed in #filelist_readjob_free, so any copied pointers need to be
	 * set to nullptr to avoid double-freeing them. */
	FileList *tmp_filelist;
} FileListReadJob;

FileList *filelist_new(int type) {
	FileList *p = MEM_callocN(sizeof(FileList), "FileList");
	filelist_settype(p, type);
	p->flags |= FL_FORCE_RESET;
	return p;
}

ROSE_INLINE void parent_dir_until_exists_or_default_root(char *dir, size_t maxncpy) {
	/* Only allow absolute paths as CWD relative doesn't make sense from the UI. */
	if (LIB_path_is_absolute_from_cwd(dir) && LIB_path_parent_dir_until_exists(dir, maxncpy)) {
		return;
	}

	LIB_path_current_working_directory(dir, maxncpy);
}

ROSE_INLINE bool filelist_checkdir_dir(const FileList *unused, char dirpath[FILE_MAX], const bool do_change) {
	bool is_valid;
	if (do_change) {
		parent_dir_until_exists_or_default_root(dirpath, FILE_MAX);
		is_valid = true;
	}
	else {
		is_valid = LIB_path_is_absolute_from_cwd(dirpath) && LIB_is_dir(dirpath);
	}
	return is_valid;
}

ROSE_INLINE void filelist_readjob_recursive_dir_add_items(FileListReadJob *job_params, const bool *stop, bool *do_update, float *progress) {
	/* Use the thread-safe filelist queue. */
	FileList *filelist = job_params->tmp_filelist;

	ListBase entries;
	LIB_listbase_clear(&entries);

	char dir[FILE_MAX];
	const char *root = filelist->filelist.root;
	LIB_strcpy(dir, ARRAY_SIZE(dir), root);

	DirEntry *e = NULL;
	size_t n = LIB_filelist_dir_contents(dir, &e);

	for (size_t index = 0; index < n; index++) {
		if (FILENAME_IS_CURRENT(e[index].name)) {
			continue;
		}

		FileDirEntry *entry = MEM_callocN(sizeof(FileDirEntry), "FileDirEntry");

		LIB_strcpy(entry->name, ARRAY_SIZE(entry->name), e[index].name);
		LIB_strcpy(entry->relpath, ARRAY_SIZE(entry->relpath), e[index].name);
		entry->time = e[index].info.st_atime;
		entry->size = e[index].info.st_size;
		
		if (e[index].type == RDT_DIR) {
			entry->type |= FILE_TYPE_DIR;
		}

		LIB_addtail(&entries, entry);
	}

	MEM_SAFE_FREE(e);

	LISTBASE_FOREACH_MUTABLE(FileDirEntry *, entry, &entries) {
		LIB_remlink(&entries, entry);
		/** Move the entry from local #entries to the filelist! */
		LIB_addtail(&job_params->tmp_filelist->filelist.entries, entry);
		job_params->tmp_filelist->filelist.totentries++;
	}
}

ROSE_INLINE void filelist_readjob_do(FileListReadJob *job_params, const bool *stop, bool *do_update, float *progress) {
	/* Use the thread-safe filelist queue. */
	FileList *filelist = job_params->tmp_filelist;
	ROSE_assert(LIB_listbase_is_empty(&filelist->filelist.entries) && (filelist->filelist.totentries == FILEDIR_NBR_ENTRIES_UNSET));

	filelist->filelist.totentries = 0;
	filelist_readjob_recursive_dir_add_items(job_params, stop, do_update, progress);
}

ROSE_INLINE void filelist_readjob_dir(FileListReadJob *job_params, bool *stop, bool *do_update, float *progress) {
	filelist_readjob_do(job_params, stop, do_update, progress);
}

void filelist_settype(FileList *filelist, int type) {
	if (filelist->type == type) {
		return;
	}

	filelist->type = type;
	switch (filelist->type) {
		case FILE_ROSE: {
			filelist->check_dir_fn = filelist_checkdir_dir;
			filelist->read_job_fn = filelist_readjob_dir;
		} break;
		default: {
			ROSE_assert_unreachable();
		} break;
	}
}

ROSE_INLINE void filelist_intern_entry_free(FileDirEntry *entry) {
	MEM_freeN(entry);
}

ROSE_INLINE void filelist_intern_free(FileDirEntryArr *filelist) {
	LISTBASE_FOREACH_MUTABLE(FileDirEntry *, entry, &filelist->entries) {
		filelist_intern_entry_free(entry);
	}
	LIB_listbase_clear(&filelist->entries);

	filelist->totentries = FILEDIR_NBR_ENTRIES_UNSET;
}

void filelist_clear(FileList *filelist) {
	filelist_intern_free(&filelist->filelist);
	filelist_intern_free(&filelist->filelist_intern);
}

typedef struct FileSortData {
	unsigned int is_inverted : 1;
} FileSortData;

ROSE_INLINE int compare_direntry_generic(const FileDirEntry *entry1, const FileDirEntry *entry2) {
	if (entry1->type & FILE_TYPE_DIR) {
		if (entry2->type & FILE_TYPE_DIR) {
			return 0;
		}
		else {
			return -1;
		}
	}
	else if (entry2->type & FILE_TYPE_DIR) {
		return 1;
	}

	/* make sure "." and ".." are always first */
	if (FILENAME_IS_CURRENT(entry1->relpath)) {
		return -1;
	}
	if (FILENAME_IS_CURRENT(entry2->relpath)) {
		return 1;
	}
	if (FILENAME_IS_PARENT(entry1->relpath)) {
		return -1;
	}
	if (FILENAME_IS_PARENT(entry2->relpath)) {
		return 1;
	}

	return 0;
}

ROSE_INLINE int compare_apply_inverted(int val, const FileSortData *sort_data) {
	return sort_data->is_inverted ? -val : val;
}

ROSE_INLINE int compare_name(const void *link1, const void *link2, void *user_data) {
	const FileSortData *sort_data = (const FileSortData *)user_data;
	const FileDirEntry *entry1 = (const FileDirEntry *)(link1);
	const FileDirEntry *entry2 = (const FileDirEntry *)(link2);

	int ret;
	if ((ret = compare_direntry_generic(entry1, entry2))) {
		return ret;
	}

	return compare_apply_inverted(strcmp(entry1->name, entry2->name), sort_data);
}

void filelist_sort(FileList *filelist) {
	if ((filelist->flags & FL_IS_SORTED) != 0) {
		return;
	}

	int (*cb)(const void *, const void *, void *) = NULL;

	switch (0) {
		default: {
			cb = compare_name;
		} break;
	}

	FileSortData sort_data = {
		.is_inverted = false,
	};

	if (cb) {
		LIB_listbase_sort(&filelist->filelist_intern.entries, cb, &sort_data);
	}

	filelist->flags |= FL_IS_SORTED;
}

void filelist_free(FileList *filelist) {
	filelist_clear(filelist);

	MEM_freeN(filelist);
}

const char *filelist_dir(const FileList *filelist) {
	return filelist->filelist.root;
}

bool filelist_is_dir(const FileList *filelist, const char *path) {
	return filelist->check_dir_fn(filelist, (char *)path, false);
}

void filelist_setdir(FileList *filelist, char dirpath[FILE_MAX]) {
	filelist->check_dir_fn(filelist, dirpath, true);

	if (!STREQ(filelist->filelist.root, dirpath)) {
		LIB_strcpy(filelist->filelist.root, ARRAY_SIZE(filelist->filelist.root), dirpath);
		filelist->flags |= FL_FORCE_RESET;
	}
}

FileDirEntry *filelist_file_ex(FileList *filelist, const size_t query, const bool use_request) {
	if ((query < 0) || (query >= filelist->filelist_intern.totentries)) {
		return NULL;
	}

	size_t index;
	LISTBASE_FOREACH_INDEX(FileDirEntry *, e, &filelist->filelist_intern.entries, index) {
		if (index == query) {
			return e;
		}
	}

	return NULL;
}

FileDirEntry *filelist_file(FileList *filelist, size_t index) {
	return filelist_file_ex(filelist, index, true);
}

void filelist_file_set(rContext *C, SpaceFile *sfile, size_t index) {
	const FileDirEntry *file = filelist_file(sfile->files, index);
	if (file && file->relpath && file->relpath[0] && !(file->type & FILE_TYPE_DIR)) {
		FileSelectParams *params = ED_fileselect_get_active_params(sfile);
		LIB_strcpy(params->file, ARRAY_SIZE(params->file), file->relpath);
		if (sfile->op) {
			/* Update the filepath properties of the operator. */
			Main *main = CTX_data_main(C);
			file_sfile_to_operator(C, main, sfile->op, sfile);
		}
	}
}

size_t filelist_files_ensure(FileList *filelist) {
	if (!filelist_needs_force_reset(filelist) || !filelist_needs_reading(filelist)) {
		filelist_sort(filelist);
	}

	return filelist->filelist.totentries;
}

size_t filelist_needs_reading(const FileList *filelist) {
	return (filelist->filelist.totentries == FILEDIR_NBR_ENTRIES_UNSET) || filelist_needs_force_reset(filelist);
}

bool filelist_needs_force_reset(const FileList *filelist) {
	return (filelist->flags & (FL_FORCE_RESET)) != 0;
}

void filelist_tag_force_reset(FileList *filelist) {
	filelist->flags |= FL_FORCE_RESET;
}

bool filelist_ready(const FileList *filelist) {
	return (filelist->flags & FL_IS_READY) != 0;
}

bool filelist_pending(const FileList *filelist) {
	return (filelist->flags & FL_IS_PENDING) != 0;
}

/**
 * \note This may trigger partial filelist reading. If the #FL_FORCE_RESET_MAIN_FILES flag is set,
 *       some current entries are kept and we just call the readjob to update the main files (see
 *       #FileListReadJob.only_main_data).
 */
ROSE_INLINE void filelist_readjob_startjob(void *vflrj, wmJobWorkerStatus *worker_status) {
	FileListReadJob *flrj = (FileListReadJob *)vflrj;

	LIB_spin_lock(&flrj->lock);
	ROSE_assert((flrj->tmp_filelist == NULL) && flrj->filelist);
	flrj->tmp_filelist = (FileList *)MEM_dupallocN(flrj->filelist);
	LIB_listbase_clear(&flrj->tmp_filelist->filelist.entries);
	LIB_listbase_clear(&flrj->tmp_filelist->filelist_intern.entries);
	flrj->tmp_filelist->filelist.totentries = FILEDIR_NBR_ENTRIES_UNSET;
	flrj->tmp_filelist->read_job_fn(flrj, &worker_status->stop, &worker_status->do_update, &worker_status->progress);
	LIB_spin_unlock(&flrj->lock);
}

/**
 * \note This may update for a partial filelist reading job. If the #FL_FORCE_RESET_MAIN_FILES flag
 *       is set, some current entries are kept and we just call the readjob to update the main
 *       files (see #FileListReadJob.only_main_data).
 */
ROSE_INLINE void filelist_readjob_update(void *flrjv) {
	FileListReadJob *flrj = (FileListReadJob *)(flrjv);

	LIB_spin_lock(&flrj->lock);
	if (flrj->tmp_filelist->filelist.totentries > 0) {
		LIB_freelistN(&flrj->filelist->filelist.entries);
		LIB_freelistN(&flrj->filelist->filelist_intern.entries);

		LIB_duplicatelist(&flrj->filelist->filelist.entries, &flrj->tmp_filelist->filelist.entries);
		LIB_duplicatelist(&flrj->filelist->filelist_intern.entries, &flrj->tmp_filelist->filelist.entries);
		flrj->filelist->filelist.totentries = flrj->filelist->filelist_intern.totentries = flrj->tmp_filelist->filelist.totentries;

		// Empty the data from the temporary file list!
		LIB_freelistN(&flrj->tmp_filelist->filelist.entries);
		flrj->tmp_filelist->filelist.totentries = 0;
	}

	if (flrj->filelist->filelist.totentries > 0) {
		flrj->filelist->flags &= ~FL_IS_SORTED;
	}
	LIB_spin_unlock(&flrj->lock);
}

ROSE_INLINE void filelist_readjob_endjob(void *flrjv) {
	FileListReadJob *flrj = (FileListReadJob *)(flrjv);

	/* In case there would be some dangling update... */
	filelist_readjob_update(flrjv);

	flrj->filelist->flags &= ~FL_IS_PENDING;
	flrj->filelist->flags |= FL_IS_READY;
}

ROSE_INLINE void filelist_readjob_free(void *flrjv) {
	FileListReadJob *flrj = (FileListReadJob *)(flrjv);

	if (flrj->tmp_filelist) {
		ROSE_assert(flrj->tmp_filelist->filelist.totentries == 0);
		ROSE_assert(LIB_listbase_is_empty(&flrj->tmp_filelist->filelist.entries));

		filelist_free(flrj->tmp_filelist);
	}

	MEM_freeN(flrj);
}

ROSE_INLINE void filelist_readjob_start_ex(FileList *filelist, const rContext *C, const bool force_blocking_read) {
	Main *main = CTX_data_main(C);

	if (!filelist_is_dir(filelist, filelist->filelist.root)) {
		return;
	}

	FileListReadJob *flrj = MEM_callocN(sizeof(FileListReadJob), "FileListReadJob");

	LIB_spin_init(&flrj->lock);

	flrj->filelist = filelist;
	flrj->main = main;

	filelist->flags &= ~(FL_FORCE_RESET | FL_IS_READY);
	filelist->flags |= FL_IS_PENDING;

	const bool no_threads = (filelist->tags & FILELIST_TAGS_NO_THREADS);

	if (force_blocking_read || no_threads) {
		/* Single threaded execution. Just directly call the callbacks. */
		wmJobWorkerStatus worker_status;
		memset(&worker_status, 0, sizeof(wmJobWorkerStatus));

		filelist_readjob_startjob(flrj, &worker_status);
		filelist_readjob_endjob(flrj);
		filelist_readjob_free(flrj);

		return;
	}

	ROSE_assert_unreachable();
}

void filelist_readjob_start(FileList *filelist, const rContext *C) {
	filelist_readjob_start_ex(filelist, C, true);
}
