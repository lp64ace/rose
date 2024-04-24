#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum {
	ROSE_LOADER_WRITE_DATABLOCKS = (1 << 0),
};

bool RLO_write_file(struct Main *main, const char *filepath, int flags);

#ifdef __cplusplus
}
#endif
