#define _CRT_SECURE_NO_WARNINGS
#include "MEM_alloc.h"

#include "LIB_fileops.h"
#include "LIB_string.h"
#include "LIB_sys_types.h"
#include "LIB_utildefines.h"

#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>

#ifdef _WIN32
#    include <windows.h>
#    include <io.h>
#    include <fileapi.h>
#else
#    include <dirent.h>
#    include <sys/param.h>
#    include <sys/wait.h>
#    include <unistd.h>
#endif

int LIB_exists(const char *path) {
#if defined(WIN32)
    if (_access(path, 0) == 0) {
        return true;
    }
#else
    if (access(path, F_OK) != -1) {
        return true;
    }
#endif
    return false;
}

FILE *LIB_fopen(const char *filepath, const char *mode) {
    return fopen(filepath, mode);
}

int64_t LIB_ftell(FILE *stream) {
#if defined(_WIN32) || defined(WIN32)
    return _ftelli64(stream);
#else
    return ftell(stream);
#endif
}

int LIB_fseek(FILE *stream, int64_t offset, int whence) {
#if defined(_WIN32) || defined(WIN32)
    return _fseeki64(stream, offset, whence);
#else
    return fseek(stream, offset, whence);
#endif
}

static bool delete_unique(const char *path, const bool dir) {
    bool err;

    if (dir) {
		err = !RemoveDirectory(path);
        if (err) {
            printf("Unable to remove directory\n");
        }
    }
    else {
		err = !DeleteFile(path);
        if (err) {
            printf("Unable to delete file\n");
        }
    }

    return err;
}

static bool delete_recursive(const char *dir) {
    return true;
}

int LIB_delete(const char *file, bool dir, bool recursive) {
    int err;

    if (recursive) {
        err = delete_recursive(file);
    }
    else {
        err = delete_unique(file, dir);
    }

    return err;
}
