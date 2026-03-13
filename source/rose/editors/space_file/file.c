#include "DNA_space_types.h"

#include "ED_fileselect.h"
#include "ED_screen.h"

#include "RNA_access.h"

#include "LIB_string.h"
#include "LIB_path_utils.h"

#include "WM_api.h"
#include "WM_handler.h"
#include "WM_window.h"

#include "filelist/filelist.h"
#include "file.h"

/* -------------------------------------------------------------------- */
/** \name Public File Select Methods
 * \{ */

void ED_fileselect_set_params_from_userdef(SpaceFile *sfile) {
	// TODO;
}

void ED_fileselect_params_to_userdef(SpaceFile *sfile) {
	// TODO;
}

ScrArea *ED_fileselect_handler_area_find(const wmWindow *win, const wmOperator *file_operator) {
	Screen *screen = WM_window_get_active_screen(win);

	ED_screen_areas_iter(win, screen, area) {
		if (area->spacetype == SPACE_FILE) {
			SpaceFile *sfile = (SpaceFile *)(area->spacedata.first);

			if (sfile->op == file_operator) {
				return area;
			}
		}
	}

	return NULL;
}

ScrArea *ED_fileselect_handler_area_find_any_with_op(const wmWindow *win) {
	const Screen *screen = WM_window_get_active_screen(win);

	ED_screen_areas_iter(win, screen, area) {
		if (area->spacetype != SPACE_FILE) {
			continue;
		}

		const SpaceFile *sfile = (SpaceFile *)(area->spacedata.first);
		if (sfile->op) {
			return area;
		}
	}

	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Select Params
 * \{ */

ROSE_INLINE void fileselect_initialize_params_common(SpaceFile *sfile, FileSelectParams *params) {
	if (!params->dir[0]) {
		LIB_path_current_working_directory(params->dir, ARRAY_SIZE(params->dir));
	}

	ED_folderlist_pushdir(&sfile->folders_prev, params->dir);
}

FileSelectParams *ED_fileselect_ensure_active_params(SpaceFile *file) {
	FileSelectParams *params;
	wmOperator *op = file->op;

	if (!file->params) {
		file->params = MEM_callocN(sizeof(FileSelectParams), "FileSelectParams");

		LIB_path_current_working_directory(file->params->dir, ARRAY_SIZE(file->params->dir));
	}

	params = file->params;

	if (op) {
		PropertyRNA *prop;
		const bool is_files = (RNA_struct_find_property(op->ptr, "files") != NULL);
		const bool is_filepath = (RNA_struct_find_property(op->ptr, "filepath") != NULL);
		const bool is_filename = (RNA_struct_find_property(op->ptr, "filename") != NULL);
		const bool is_directory = (RNA_struct_find_property(op->ptr, "directory") != NULL);
		const bool is_relative_path = (RNA_struct_find_property(op->ptr, "relative_path") != NULL);

		LIB_strcpy(params->title, ARRAY_SIZE(params->title), op->type->name);

		if ((prop = RNA_struct_find_property(op->ptr, "filemode"))) {
			params->type = RNA_property_int_get(op->ptr, prop);
		}
		else {
			params->type = FILE_ROSE;
		}

		if (is_filepath && RNA_struct_property_is_set_ex(op->ptr, "filepath", false)) {
			char filepath[FILE_MAX];
			RNA_string_get(op->ptr, "filepath", filepath);
		}
		else {
			if (is_directory && RNA_struct_property_is_set_ex(op->ptr, "directory", false)) {
				RNA_string_get(op->ptr, "directory", params->dir);
				params->file[0] = '\0';
			}

			if (is_filename && RNA_struct_property_is_set_ex(op->ptr, "filename", false)) {
				RNA_string_get(op->ptr, "filename", params->file);
			}
		}

		if (!params->dir[0]) {
			LIB_path_current_working_directory(file->params->dir, ARRAY_SIZE(file->params->dir));
		}

		params->flag = 0;
		if (is_directory == true && is_filename == false && is_filepath == false && is_files == false) {
			params->flag |= FILE_DIRSEL_ONLY;
		}
		if ((prop = RNA_struct_find_property(op->ptr, "check_existing"))) {
			params->flag |= RNA_property_boolean_get(op->ptr, prop) ? FILE_CHECK_EXISTING : 0;
		}
	}
	else {
		/** So that the action button will not stay without text! */
		LIB_strcpy(params->title, ARRAY_SIZE(params->title), "Open");

		params->type = FILE_ROSE;
	}

	fileselect_initialize_params_common(file, params);

	return params;
}

FileSelectParams *ED_fileselect_get_active_params(SpaceFile *file) {
	if (!file) {
		/* Sometimes called in poll before space type was checked. */
		return NULL;
	}

	return file->params;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Folder List
 * \{ */

typedef struct FolderList {
	struct FolderList *prev, *next;

	char *foldername;
} FolderList;

void ED_folderlist_popdir(ListBase *lb, char *dir) {
	const char *prev_dir;
	FolderList *folder;
	folder = (FolderList *)(lb->last);

	if (folder) {
		/* remove the current directory */
		MEM_freeN(folder->foldername);
		LIB_freelinkN(lb, folder);

		folder = (FolderList *)(lb->last);
		if (folder) {
			prev_dir = folder->foldername;
			LIB_strcpy(dir, FILE_MAXDIR, prev_dir);
		}
	}
}

void ED_folderlist_pushdir(ListBase *lb, char *dir) {
	if (!dir[0]) {
		return;
	}

	FolderList *folder, *previous_folder;
	previous_folder = (FolderList *)(lb->last);

	/* check if already exists */
	if (previous_folder && previous_folder->foldername) {
		if (LIB_path_cmp(previous_folder->foldername, dir) == 0) {
			return;
		}
	}

	/* create next folder element */
	folder = MEM_callocN(sizeof(FolderList), __func__);
	folder->foldername = LIB_strdupN(dir);

	/* add it to the end of the list */
	LIB_addtail(lb, folder);
}

bool folderlist_clear_next(SpaceFile *sfile) {
	const FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	FolderList *folder;

	/* if there is no folder_next there is nothing we can clear */
	if (LIB_listbase_is_empty(&sfile->folders_next)) {
		return false;
	}

	/* if previous_folder, next_folder or refresh_folder operators are executed
	 * it doesn't clear folder_next */
	folder = (FolderList *)(sfile->folders_prev.last);
	if ((!folder) || (LIB_path_cmp(folder->foldername, params->dir) == 0)) {
		return false;
	}

	/* eventually clear flist->folders_next */
	return true;
}

void ED_folderlist_free(ListBase *lb) {
	LISTBASE_FOREACH_MUTABLE(FolderList *, folder, lb) {
		MEM_freeN(folder->foldername);
		MEM_freeN(folder);
	}
	LIB_listbase_clear(lb);
}

void ED_folder_history_list_free(SpaceFile *file) {
	ED_folderlist_free(&file->folders_prev);
	ED_folderlist_free(&file->folders_next);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Select Params
 * \{ */

void ED_fileselect_change_dir_ex(rContext *C, ScrArea *area) {
	SpaceFile *sfile = (SpaceFile *)(area->spacedata.first);
	FileSelectParams *params = ED_fileselect_get_active_params(sfile);
	if (params) {
		WindowManager *wm = CTX_wm_manager(C);
		if (!filelist_is_dir(sfile->files, params->dir)) {
			LIB_strcpy(params->dir, ARRAY_SIZE(params->dir), filelist_dir(sfile->files));
			/* could return but just refresh the current dir */
		}
		filelist_setdir(sfile->files, params->dir);
		file_draw_check_ex(C, area);

		if (folderlist_clear_next(sfile)) {
			ED_folderlist_free(&sfile->folders_next);
		}

		ED_folderlist_pushdir(&sfile->folders_prev, params->dir);
		ED_area_tag_redraw(area);
	}
}

void ED_fileselect_change_dir(rContext *C) {
	ScrArea *area = CTX_wm_area(C);
	ED_fileselect_change_dir_ex(C, area);
}

/** \} */
