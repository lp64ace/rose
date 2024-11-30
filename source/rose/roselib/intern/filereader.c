#include "MEM_guardedalloc.h"

#include "LIB_fileops.h"
#include "LIB_filereader.h"
#include "LIB_utildefines.h"

typedef struct RawFileReader {
	FileReader reader;
	
	int descr;
} RawFileReader;

ROSE_STATIC uint64_t file_read(FileReader *reader, void *buffer, size_t size) {
	RawFileReader *rawreader = (RawFileReader *)reader;
	uint64_t readsize = LIB_read(rawreader->descr, buffer, size);
	
	if (readsize >= 0) {
		reader->offset += readsize;
	}
	
	return readsize;
}

ROSE_STATIC uint64_t file_seek(FileReader *reader, uint64_t offset, int whence) {
	RawFileReader *rawreader = (RawFileReader *)reader;
	reader->offset = LIB_seek(rawreader->descr, offset, whence);
	return reader->offset;
}

ROSE_STATIC void file_close(FileReader *reader) {
	RawFileReader *rawreader = (RawFileReader *)reader;
	close(rawreader->descr);
	MEM_freeN(rawreader);
}

FileReader *LIB_filereader_new_file(int descr) {
	RawFileReader *rawreader = MEM_mallocN(sizeof(RawFileReader), "RawFileReader");
	
	rawreader->descr = descr;
	rawreader->reader.read = file_read;
	rawreader->reader.seek = file_seek;
	rawreader->reader.close = file_close;
	file_seek((FileReader *)rawreader, 0, SEEK_SET);
	
	return (FileReader *)rawreader;
}
