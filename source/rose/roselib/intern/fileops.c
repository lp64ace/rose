#include "MEM_guardedalloc.h"

#include "LIB_assert.h"
#include "LIB_fileops.h"
#include "LIB_path_utils.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include <limits.h>
#include <stdio.h>

#if defined(WIN32)
#	include <windows.h>
#else
#	include <dirent.h>
#endif

/* -------------------------------------------------------------------- */
/** \name File Utils
 * \{ */

#ifdef WIN32
int LIB_stat(const char *filepath, RoseFileStat *r_stat) {
#	if defined(_MSC_VER)
	return _stat64(filepath, (struct _stat64 *)r_stat);
#	else
	return _stat(filepath, (struct _stat *)r_stat);
#	endif
}
#else
int LIB_stat(const char *filepath, RoseFileStat *r_stat) {
	return stat(filepath, r_stat);
}
#endif

int LIB_open(const char *filepath, int oflag, int pmode) {
	return open(filepath, oflag, pmode);
}

uint64_t LIB_read(int fd, void *buffer, size_t size) {
	uint64_t nbytes_read_total = 0;
	while (true) {
		int nbytes = read(fd, buffer, ROSE_MIN(size, INT_MAX));
		if (nbytes == size) {
			return nbytes_read_total + nbytes;
		}
		if (nbytes == 0) {
			return nbytes_read_total;
		}
		if (nbytes < 0) {
			return nbytes;
		}
		if (nbytes > size) {
			ROSE_assert_unreachable();
		}

		buffer = POINTER_OFFSET(buffer, nbytes);
		nbytes_read_total += nbytes;
		size -= nbytes;
	}
}

uint64_t LIB_seek(int fd, size_t offset, int whence) {
#ifdef WIN32
	return _lseeki64(fd, offset, whence);
#else
	return lseek(fd, offset, whence);
#endif
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File List
 * \{ */

#ifndef MAX_PATH
#	define MAX_PATH 1024
#endif

#if defined(WIN32)

ROSE_STATIC size_t lib_filelist_path(char out[MAX_PATH], const char *dirname) {
	if (!LIB_path_absolute(out, MAX_PATH, dirname)) {
		return 0;
	}
	size_t length = LIB_strnlen(out, MAX_PATH);
	ROSE_assert(length + 3 < MAX_PATH);
	if (!ELEM(out[length - 1], SEP, ALTSEP)) {
		out[length++] = SEP;  // defined in "LIB_path_utils.h"
	}
	out[length++] = '*';
	out[length++] = '\0';
	return length;
}

ROSE_STATIC int lib_filelist_type(WIN32_FIND_DATA *e) {
	if (e->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		return RDT_DIR;
	}
	if (e->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
		return RDT_LNK;
	}
	return RDT_REG;
}

ROSE_STATIC size_t lib_filelist_dir_contents_win32(const char *dirname, DirEntry **r_list) {
	char absolute[MAX_PATH];
	lib_filelist_path(absolute, dirname);

	WIN32_FIND_DATA e;

	HANDLE hFind = FindFirstFile(absolute, &e);
	if (hFind == INVALID_HANDLE_VALUE) {
		return -1;
	}

	char full[MAX_PATH];

	size_t count = 0;
	do {
		size_t index = count++;
		(*r_list) = MEM_recallocN_id(*r_list, sizeof(DirEntry) * count, "DirEntryArray");

		(*r_list)[index].type = lib_filelist_type(&e);
		if ((*r_list)[index].type == RDT_REG) {
			LIB_path_join(full, MAX_PATH, dirname, SEP_STR, e.cFileName);
			LIB_stat(full, &(*r_list)[index].info);
		}
		LIB_strcpy((*r_list)[index].name, ARRAY_SIZE((*r_list)[index].name), e.cFileName);
	} while (FindNextFile(hFind, &e));

	return count;
}
#else
ROSE_STATIC int lib_filelist_type(struct dirent *e) {
#	define MAP(unix, rose) \
		case unix:          \
			return rose;
	switch (e->d_type) {
		MAP(DT_REG, RDT_REG);
		MAP(DT_DIR, RDT_DIR);
		MAP(DT_LNK, RDT_LNK);
	}

#	undef MAP

	return 0;
}

ROSE_STATIC size_t lib_filelist_dir_contents_unix(const char *dirname, DirEntry **r_list) {
	DIR *DirFindData = opendir(dirname);
	if (!DirFindData) {
		return -1;
	}

	char full[MAX_PATH];

	size_t count = 0;
	for (struct dirent *e = NULL; e = readdir(DirFindData);) {
		size_t index = count++;
		(*r_list) = MEM_reallocN_id(*r_list, sizeof(DirEntry) * count, "DirEntryArray");

		(*r_list)[index].type = lib_filelist_type(e);
		if ((*r_list)[index].type == RDT_REG) {
			LIB_path_join(full, MAX_PATH, dirname, SEP_STR, e->d_name);
			LIB_stat(full, &(*r_list)[index].info);
		}
		LIB_strcpy((*r_list)[index].name, ARRAY_SIZE((*r_list)[index].name), e->d_name);
	}

	closedir(DirFindData);
	return count;
}
#endif

size_t LIB_filelist_dir_contents(const char *dirname, DirEntry **r_list) {
#if defined(WIN32)
	return lib_filelist_dir_contents_win32(dirname, r_list);
#else
	return lib_filelist_dir_contents_unix(dirname, r_list);
#endif
}

/** \} */
