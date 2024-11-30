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

#ifdef __cplusplus
extern "C" {
#endif

int LIB_open(const char *filepath, int oflag, int pmode);
uint64_t LIB_read(int fd, void *buffer, size_t size);
uint64_t LIB_seek(int fd, size_t offset, int whence);

#ifdef __cplusplus
}
#endif

#endif	// LIB_FILEOPS_H
