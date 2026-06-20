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

bool LIB_file_magic_is_zstd(const char header[4]) {
	/* ZSTD files consist of concatenated frames, each either a ZSTD frame or a skippable frame.
	 * Both types of frames start with a magic number: `0xFD2FB528` for ZSTD frames and `0x184D2A5`
	 * for skippable frames, with the * being anything from 0 to F.
	 *
	 * To check whether a file is ZSTD-compressed, we just check whether the first frame matches
	 * either. Seeking through the file until a ZSTD frame is found would make things more
	 * complicated and the probability of a false positive is rather low anyways.
	 *
	 * Note that LZ4 uses a compatible format, so even though its compressed frames have a
	 * different magic number, a valid LZ4 file might also start with a skippable frame matching
	 * the second check here.
	 *
	 * For more details, see https://github.com/facebook/zstd/blob/dev/doc/zstd_compression_format.md
	 */

	uint32_t magic = *((uint32_t *)((char *)(header)));
	if (magic == 0xFD2FB528) {
		return true;
	}
	if ((magic >> 4) == 0x184D2A5) {
		return true;
	}
	return false;
}
