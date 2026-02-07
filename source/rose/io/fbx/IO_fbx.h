#ifndef IO_FBX_H
#define IO_FBX_H

struct rContext;

#ifdef __cplusplus
extern "C" {
#endif

void FBX_import(struct rContext *C, const char *filepath, float unit);
void FBX_import_memory(struct rContext *C, const void *memory, size_t size, float unit);

#ifdef __cplusplus
}
#endif

#endif
