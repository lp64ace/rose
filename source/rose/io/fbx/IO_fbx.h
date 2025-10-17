#ifndef IO_FBX_H
#define IO_FBX_H

struct rContext;

#ifdef __cplusplus
extern "C" {
#endif

void FBX_import(struct rContext *C, const char *filepath);

#ifdef __cplusplus
}
#endif

#endif
