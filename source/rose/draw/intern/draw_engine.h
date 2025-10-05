#ifndef DRAW_ENGINE_H
#define DRAW_ENGINE_H

#include "GPU_batch.h"
#include "GPU_framebuffer.h"
#include "GPU_texture.h"
#include "GPU_viewport.h"

#include "LIB_memblock.h"
#include "LIB_mempool.h"

#include "DRW_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Draw Resource Handle & Data
 * \{ */

#define DRW_RESOURCE_CHUNK_LEN (1 << 9)

typedef unsigned int DRWResourceHandle;

typedef struct DRWObjectMatrix {
	float model[4][4];
	float modelinverse[4][4];
} DRWObjectMatrix;

ROSE_INLINE DRWResourceHandle DRW_handle_increment(DRWResourceHandle *handle) {
	return ++(*handle) - 1;
}

ROSE_INLINE size_t DRW_handle_chunk_get(const DRWResourceHandle *handle) {
	return (*handle & 0x7fffffff) >> 9;
}

ROSE_INLINE size_t DRW_handle_elem_get(const DRWResourceHandle *handle) {
	return (*handle) & 0x1ff;
}

ROSE_INLINE void *DRW_memblock_elem_from_handle(struct MemBlock *memblock, const DRWResourceHandle *handle) {
	return LIB_memory_block_elem_get(memblock, DRW_handle_chunk_get(handle), DRW_handle_elem_get(handle));
}

void DRW_render_buffer_finish();

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Engines Data
 * \{ */

enum {
	DRW_UNIFORM_INT = 0,
	DRW_UNIFORM_INT_COPY,
	DRW_UNIFORM_FLOAT,
	DRW_UNIFORM_FLOAT_COPY,
	DRW_UNIFORM_TEXTURE,
	DRW_UNIFORM_TEXTURE_REF,
	DRW_UNIFORM_IMAGE,
	DRW_UNIFORM_IMAGE_REF,
	DRW_UNIFORM_BLOCK,
	DRW_UNIFORM_BLOCK_REF,
	DRW_UNIFORM_STORAGE_BLOCK,
	DRW_UNIFORM_STORAGE_BLOCK_REF,
	DRW_UNIFORM_TFEEDBACK_TARGET,
	DRW_UNIFORM_VERTEX_BUFFER_AS_TEXTURE,
	DRW_UNIFORM_VERTEX_BUFFER_AS_TEXTURE_REF,
	DRW_UNIFORM_VERTEX_BUFFER_AS_STORAGE,
	DRW_UNIFORM_VERTEX_BUFFER_AS_STORAGE_REF,

	// Builtin

	DRW_UNIFORM_RESOURCE_ID,
	DRW_UNIFORM_BLOCK_OBMATS,
};

typedef struct DRWUniform {
	struct DRWUniform *prev, *next;

	int location;
	int type;
	int veclen;
	int arrlen;

	union {
		void *pvalue;
		float fvalue[4];
		int ivalue[4];

		struct {
			union {
				GPUTexture *texture;
				GPUTexture **ptexture;
			};
			GPUSamplerState sampler;
		};
		union {
			GPUUniformBuf *block;
			GPUUniformBuf **pblock;
		};
		union {
			GPUStorageBuf *ssbo;
			GPUStorageBuf **pssbo;
		};
		union {
			GPUVertBuf *vertbuf;
			GPUVertBuf **pvertbuf;
		};
	};
} DRWUniform;

typedef struct DRWCommandClear {
	unsigned char bits;
	unsigned char stencil;
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
	float depth;
} DRWCommandClear;

typedef struct DRWCommandDraw {
	struct GPUBatch *batch;
	unsigned int vcount;
} DRWCommandDraw;

typedef struct DRWCommandDrawRange {
	struct GPUBatch *batch;
	unsigned int vfirst;
	unsigned int vcount;
} DRWCommandDrawRange;

typedef struct DRWCommandDrawInstance {
	struct GPUBatch *batch;
	unsigned int icount;
} DRWCommandDrawInstance;

typedef struct DRWCommandDrawInstanceRange {
	struct GPUBatch *batch;
	unsigned int ifirst;
	unsigned int icount;
} DRWCommandDrawInstanceRange;

enum {
	DRW_COMMAND_CLEAR,
	DRW_COMMAND_DRAW,
	DRW_COMMAND_DRAW_RANGE,
	DRW_COMMAND_DRAW_INSTANCE,
	DRW_COMMAND_DRAW_INSTANCE_RANGE,
};

typedef struct DRWCommand {
	struct DRWCommand *prev, *next;

	int type;

	union {
		struct DRWCommandClear clear;
		struct DRWCommandDraw draw;
		struct DRWCommandDrawRange draw_range;
		struct DRWCommandDrawInstance draw_instance;
		struct DRWCommandDrawInstanceRange draw_instance_range;
	};

	DRWResourceHandle handle;
} DRWCommand;

typedef struct DRWShadingGroup {
	struct DRWShadingGroup *prev, *next;

	struct GPUShader *shader;

	ListBase uniforms;	// #DRWUniform
	ListBase commands;	// #DRWCommand
} DRWShadingGroup;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw View Engine Data
 * \{ */

struct GPUFrameBuffer;
struct GPUTexture;
struct DRWPass;

typedef struct DRWViewportEngineDataFramebufferList {
	GPUFrameBuffer *framebuffers[1];
} DRWViewportEngineDataFramebufferList;

typedef struct DRWViewportEngineDataTextureList {
	GPUTexture *textures[1];
} DRWViewportEngineDataTextureList;

typedef struct DRWViewportEngineDataPassList {
	DRWPass *passes[1];
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
