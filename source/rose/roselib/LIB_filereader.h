#ifndef LIB_FILEREADER_H
#define LIB_FILEREADER_H

#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

struct FileReader;

typedef uint64_t (*FileReaderReadFn)(struct FileReader *reader, void *buffer, size_t size);
typedef uint64_t (*FileReaderSeekFn)(struct FileReader *reader, uint64_t off, int whence);
typedef void (*FileReaderCloseFn)(struct FileReader *reader);

typedef struct FileReader {
	FileReaderReadFn read;
	FileReaderSeekFn seek;
	FileReaderCloseFn close;
	
	uint64_t offset;
} FileReader;

/** Raw byte read from simple native file descriptor! */
FileReader *LIB_filereader_new_file(int descr);

#ifdef __cplusplus
}
#endif

#endif // LIB_FILEREADER_H