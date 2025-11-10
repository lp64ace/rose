#ifndef DRAW_PASS_H
#define DRAW_PASS_H

#include "GPU_texture.h"

#include "LIB_memblock.h"
#include "LIB_listbase.h"

#include "draw_state.h"

/* -------------------------------------------------------------------- */
/** \name Draw Resource Handle & Data
 * \{ */

#define DRW_RESOURCE_CHUNK_LEN (1 << 8)

typedef unsigned int DRWResourceHandle;

typedef struct DRWObjectMatrix {
	float model[4][4];
	float modelinverse[4][4];
	float armature[4][4];
	float armatureinverse[4][4];
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
/** \name Draw Shading Group
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
		const void *pvalue;
		float fvalue[4];
		int ivalue[4];

		struct {
			union {
				struct GPUTexture *texture;
				struct GPUTexture **ptexture;
			};
			GPUSamplerState sampler;
		};
		union {
			struct GPUUniformBuf *block;
			struct GPUUniformBuf **pblock;
		};
		union {
			struct GPUStorageBuf *ssbo;
			struct GPUStorageBuf **pssbo;
		};
		union {
			struct GPUVertBuf *vertbuf;
			struct GPUVertBuf **pvertbuf;
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
/** \name Draw Deform Vert
 * \{ */

typedef struct DRWDVertGroupInfo {
	/**
	 * The uniform buffer that stores the matrices for ecah deform group.
	 */
	GPUUniformBuf *matrices;
} DRWDVertGroupInfo;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Draw Pass
 * \{ */

typedef struct DRWShadingGroup DRWShadingGroup;

typedef struct DRWPass {
	/**
	 * \type DRWShadingGroup each pass consists of multiple shading group rendering passes!
	 * Each group commands are executed in order.
	 */
	ListBase groups;

	/**
	 * Draw the shading group commands of the original pass instead,
	 * this way we avoid duplicating drawcalls and shading group commands for similar passes!
	 */
	struct DRWPass *original;
	/** Link list of additional passes to render! */
	struct DRWPass *next;

	char name[64];

	DRWState state;
} DRWPass;

/** \} */

#endif