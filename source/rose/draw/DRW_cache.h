#ifndef DRW_CACHE_H
#define DRW_CACHE_H

struct GPUBatch;

#ifdef __cplusplus
extern "C" {
#endif

struct GPUBatch *DRW_cache_fullscreen_quad_get(void);

void DRW_global_cache_free(void);

#ifdef __cplusplus
}
#endif

#endif
