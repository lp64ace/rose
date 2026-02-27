#include "MEM_guardedalloc.h"

#include "LIB_fileops.h"
#include "LIB_listbase.h"
#include "LIB_path_utils.h"
#include "LIB_utildefines.h"

#include "filelist.h"

FileList *filelist_new(int type) {
	FileList *p = MEM_callocN(sizeof(FileList), "FileList");
	filelist_settype(p, type);
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

void filelist_settype(FileList *filelist, int type) {
	if (filelist->type == type) {
		return;
	}

	filelist->type = type;
	switch (filelist->type) {
		case FILE_ROSE: {
			filelist->check_dir_fn = filelist_checkdir_dir;
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
}

void filelist_clear(FileList *filelist) {
	filelist_intern_free(&filelist->filelist);
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

void filelist_setdir(struct FileList *filelist, char dirpath[FILE_MAX]) {
	filelist->check_dir_fn(filelist, dirpath, true);
}
