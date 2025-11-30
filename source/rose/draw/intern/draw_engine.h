#ifndef DRAW_ENGINE_H
#define DRAW_ENGINE_H

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_texture.h"
#include "GPU_viewport.h"

#include "LIB_memblock.h"
#include "LIB_mempool.h"

#include "DRW_engine.h"

#include "shaders/draw_shader_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Draw View Engine Data
 * \{ */

struct GPUFrameBuffer;
struct GPUTexture;
struct DRWPass;

typedef struct DRWViewportEngineDataFramebufferList {
	struct GPUFrameBuffer *framebuffers[1];
} DRWViewportEngineDataFramebufferList;

typedef struct DRWViewportEngineDataTextureList {
	struct GPUTexture *textures[1];
} DRWViewportEngineDataTextureList;

typedef struct DRWViewportEngineDataPassList {
	struct DRWPass *passes[1];
} DRWViewportEngineDataPassList;

typedef struct DRWViewportEngineDataStorageList {
	void *storage[1]; // Stores custom structs from the engine that have been MEM_(m/c)allocN'ed.
} DRWViewportEngineDataStorageList;

typedef struct ViewportEngineData {
	struct ViewportEngineData *prev, *next;

	DrawEngineType *engine;

	DRWViewportEngineDataFramebufferList *fbl;
	DRWViewportEngineDataTextureList *txl;
	DRWViewportEngineDataPassList *psl;
	DRWViewportEngineDataStorageList *stl;
} ViewportEngineData;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw View Engine Data Methods
 * \{ */

typedef struct DefaultFramebufferList {
	GPUFrameBuffer *default_fb;
} DefaultFramebufferList;

typedef struct DefaultTextureList {
	GPUTexture *color;
	GPUTexture *depth;
} DefaultTextureList;

typedef struct DRWViewData {
	DefaultFramebufferList dfbl;
	DefaultTextureList dtxl;

	ViewInfos storage;

	int flag;

	int txl_size[2];

	ListBase viewport_engine_data;
} DRWViewData;

enum {
	DRW_VIEW_DATA_VIEWPORT = 1 << 0,
};

struct ViewportEngineData;

struct DRWViewData *DRW_view_data_new(struct ListBase *engines);
void DRW_view_data_free(struct DRWViewData *view_data);

struct ViewportEngineData *DRW_view_data_engine_data_get_ensure(struct DRWViewData *view_data, struct DrawEngineType *engine_type);
void DRW_view_data_texture_list_size_validate(struct DRWViewData *view_data, const int size[2]);
void DRW_view_data_default_lists_from_viewport(struct DRWViewData *view_data, struct GPUViewport *viewport);
void DRW_view_data_use_engine(struct DRWViewData *view_data, struct DrawEngineType *engine_type);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DRAW_ENGINE_H
