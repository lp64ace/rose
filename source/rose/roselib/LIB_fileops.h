#ifndef LIB_FILEOPS_H
#define LIB_FILEOPS_H

#include <fcntl.h>

#if defined(WIN32)
#	include <io.h>
#else
#	include <unistd.h>
#	if !defined(O_BINARY)
#		define O_BINARY 0
#	endif
#endif

#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32)
#	if defined(_MSC_VER)
typedef struct _stat64 RoseFileStat;
#	else
typedef struct _stat RoseFileStat;
#	endif
#else
typedef struct stat RoseFileStat;
#endif

/* -------------------------------------------------------------------- */
/** \name File Utils
 * \{ */

int LIB_stat(const char *filepath, RoseFileStat *r_stat);

int LIB_open(const char *filepath, int oflag, int pmode);
uint64_t LIB_read(int fd, void *buffer, size_t size);
uint64_t LIB_seek(int fd, size_t offset, int whence);

/** \} */

/* -------------------------------------------------------------------- */
/** \name File List
 * \{ */

typedef struct DirEntry {
	RoseFileStat info;

	int type;
	/** Null terminated string with the name of the directory. */
	char name[256];
} DirEntry;

/** #DirEntry->type */
enum {
	RDT_REG = 2,
	RDT_DIR = 4,
	RDT_LNK = 10,
};

/**
 * Scans the contents of the directory #dirname, and allocates and fills an array
 * of entries describing them.
 *
 * \return The length of the array in elements.
 */
size_t LIB_filelist_dir_contents(const char *dirname, DirEntry **r_list);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_FILEOPS_H
