#include "LIB_assert.h"
#include "LIB_fileops.h"
#include "LIB_utildefines.h"

#include <limits.h>
#include <stdio.h>

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
