#include "MEM_guardedalloc.h"

#include "GPU_batch.h"
#include "GPU_vertex_buffer.h"
#include "GPU_vertex_format.h"

#include "LIB_memblock.h"
#include "LIB_mempool.h"

#include "draw_manager.h"
#include "draw_instance_data.h"

typedef struct DRWInstanceData {
	struct DRWInstanceData *next;
	bool used;		  /* If this data is used or not. */
	size_t data_size; /* Size of one instance data. */
	MemPool *mempool;
} DRWInstanceData;

typedef struct DRWInstanceDataList {
	struct DRWInstanceDataList *next, *prev;
	/* Linked lists for all possible data pool size */
	DRWInstanceData *idata_head[MAX_INSTANCE_DATA_SIZE];
	DRWInstanceData *idata_tail[MAX_INSTANCE_DATA_SIZE];

	MemBlock *pool_instancing;
	MemBlock *pool_batching;
	MemBlock *pool_buffers;
} DRWInstanceDataList;

typedef struct DRWTempBufferHandle {
	GPUVertBuf *buffer;
	/** Format pointer for reuse. */
	GPUVertFormat *format;
	/** Touched vertex length for resize. */
	int *length;
} DRWTempBufferHandle;

typedef struct DRWTempInstancingHandle {
	/** Copy of geom but with the per-instance attributes. */
	GPUBatch *batch;
	/** Batch containing instancing attributes. */
	GPUBatch *instancer;
	/** Call-buffer to be used instead of instancer. */
	GPUVertBuf *buffer;
	/** Original non-instanced batch pointer. */
	GPUBatch *geometry;
} DRWTempInstancingHandle;

static ListBase GInstanceDataList = {NULL, NULL};

static void instancing_batch_references_add(GPUBatch *batch) {
	for (int i = 0; i < GPU_BATCH_VBO_MAX_LEN && batch->verts[i]; i++) {
		GPU_vertbuf_handle_ref_add(batch->verts[i]);
	}
	for (int i = 0; i < GPU_BATCH_INST_VBO_MAX_LEN && batch->inst[i]; i++) {
		GPU_vertbuf_handle_ref_add(batch->inst[i]);
	}
}

static void instancing_batch_references_remove(GPUBatch *batch) {
	for (int i = 0; i < GPU_BATCH_VBO_MAX_LEN && batch->verts[i]; i++) {
		GPU_vertbuf_handle_ref_remove(batch->verts[i]);
	}
	for (int i = 0; i < GPU_BATCH_INST_VBO_MAX_LEN && batch->inst[i]; i++) {
		GPU_vertbuf_handle_ref_remove(batch->inst[i]);
	}
}

/* -------------------------------------------------------------------- */
/** \name Instance Buffer Management
 * \{ */

GPUVertBuf *DRW_temp_buffer_request(DRWInstanceDataList *idatalist, GPUVertFormat *format, int *vert_len) {
	ROSE_assert(format != NULL);
	ROSE_assert(vert_len != NULL);

	DRWTempBufferHandle *handle = LIB_memory_block_alloc(idatalist->pool_buffers);

	if (handle->format != format) {
		handle->format = format;
		GPU_VERTBUF_DISCARD_SAFE(handle->buffer);

		GPUVertBuf *vert = GPU_vertbuf_calloc();
		GPU_vertbuf_init_with_format_ex(vert, format, GPU_USAGE_DYNAMIC);
		GPU_vertbuf_data_alloc(vert, DRW_BUFFER_VERTS_CHUNK);

		handle->buffer = vert;
	}
	handle->length = vert_len;
	return handle->buffer;
}

GPUBatch *DRW_temp_batch_instance_request(DRWInstanceDataList *idatalist, GPUVertBuf *buffer, GPUBatch *instancer, GPUBatch *geometry) {
	/* Do not call this with a batch that is already an instancing batch. */
	ROSE_assert(geometry->inst[0] == NULL);
	/* Only call with one of them. */
	ROSE_assert((instancer != NULL) != (buffer != NULL));

	DRWTempInstancingHandle *handle = LIB_memory_block_alloc(idatalist->pool_instancing);
	if (handle->batch == NULL) {
		handle->batch = GPU_batch_calloc();
	}

	GPUBatch *batch = handle->batch;
	bool instancer_compat = buffer ? ((batch->inst[0] == buffer) && (GPU_vertbuf_get_status(buffer) & GPU_VERTBUF_DATA_UPLOADED)) : ((batch->inst[0] == instancer->verts[0]) && (batch->inst[1] == instancer->verts[1]));
	bool is_compatible = (batch->prim_type == geometry->prim_type) && instancer_compat && (batch->flag & GPU_BATCH_BUILDING) == 0 && (batch->elem == geometry->elem);
	for (int i = 0; i < GPU_BATCH_VBO_MAX_LEN && is_compatible; i++) {
		if (batch->verts[i] != geometry->verts[i]) {
			is_compatible = false;
		}
	}

	if (!is_compatible) {
		instancing_batch_references_remove(batch);
		GPU_batch_clear(batch);
		/* Save args and init later. */
		batch->flag = GPU_BATCH_BUILDING;
		handle->buffer = buffer;
		handle->instancer = instancer;
		handle->geometry = geometry;
	}
	return batch;
}

GPUBatch *DRW_temp_batch_request(DRWInstanceDataList *idatalist, GPUVertBuf *buffer, PrimType prim_type) {
	GPUBatch **batch_ptr = LIB_memory_block_alloc(idatalist->pool_batching);
	if (*batch_ptr == NULL) {
		*batch_ptr = GPU_batch_calloc();
	}

	GPUBatch *batch = *batch_ptr;
	bool is_compatible = (batch->verts[0] == buffer) && (batch->prim_type == prim_type) && (GPU_vertbuf_get_status(buffer) & GPU_VERTBUF_DATA_UPLOADED);
	if (!is_compatible) {
		GPU_batch_clear(batch);
		GPU_batch_init(batch, prim_type, buffer, NULL);
	}
	return batch;
}

static void temp_buffer_handle_free(DRWTempBufferHandle *handle) {
	handle->format = NULL;
	GPU_VERTBUF_DISCARD_SAFE(handle->buffer);
}

static void temp_instancing_handle_free(DRWTempInstancingHandle *handle) {
	instancing_batch_references_remove(handle->batch);
	GPU_BATCH_DISCARD_SAFE(handle->batch);
}

static void temp_batch_free(GPUBatch **batch) {
	GPU_BATCH_DISCARD_SAFE(*batch);
}

void DRW_instance_buffer_finish(DRWInstanceDataList *idatalist) {
	/* Resize down buffers in use and send data to GPU. */
	MemBlockIter iter;
	DRWTempBufferHandle *handle;
	LIB_memory_block_iternew(idatalist->pool_buffers, &iter);
	while ((handle = LIB_memory_block_iterstep(&iter))) {
		if (handle->length != NULL) {
			uint vert_len = *(handle->length);
			uint target_buf_size = ((vert_len / DRW_BUFFER_VERTS_CHUNK) + 1) * DRW_BUFFER_VERTS_CHUNK;
			if (target_buf_size < GPU_vertbuf_get_vertex_alloc(handle->buffer)) {
				GPU_vertbuf_data_resize(handle->buffer, target_buf_size);
			}
			GPU_vertbuf_data_len_set(handle->buffer, vert_len);
			GPU_vertbuf_use(handle->buffer); /* Send data. */
		}
	}
	/* Finish pending instancing batches. */
	DRWTempInstancingHandle *handle_inst;
	LIB_memory_block_iternew(idatalist->pool_instancing, &iter);
	while ((handle_inst = LIB_memory_block_iterstep(&iter))) {
		GPUBatch *batch = handle_inst->batch;
		if (batch && batch->flag == GPU_BATCH_BUILDING) {
			GPUVertBuf *inst_buf = handle_inst->buffer;
			GPUBatch *inst_batch = handle_inst->instancer;
			GPUBatch *geom = handle_inst->geometry;
			GPU_batch_copy(batch, geom);
			if (inst_batch != NULL) {
				for (int i = 0; i < GPU_BATCH_INST_VBO_MAX_LEN && inst_batch->verts[i]; i++) {
					GPU_batch_instbuf_add(batch, inst_batch->verts[i], false);
				}
			}
			else {
				GPU_batch_instbuf_add(batch, inst_buf, false);
			}
			/* Add reference to avoid comparing pointers (in DRW_temp_batch_request) that could
			 * potentially be the same. This will delay the freeing of the GPUVertBuf itself. */
			instancing_batch_references_add(batch);
		}
	}
	/* Resize pools and free unused. */
	LIB_memory_block_clear(idatalist->pool_buffers, (MemblockValFreeFP)temp_buffer_handle_free);
	LIB_memory_block_clear(idatalist->pool_instancing, (MemblockValFreeFP)temp_instancing_handle_free);
	LIB_memory_block_clear(idatalist->pool_batching, (MemblockValFreeFP)temp_batch_free);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Instance Data (DRWInstanceData)
 * \{ */

static DRWInstanceData *draw_instance_data_create(DRWInstanceDataList *idatalist, uint attr_size) {
	DRWInstanceData *idata = MEM_callocN(sizeof(DRWInstanceData), "DRWInstanceData");
	idata->next = NULL;
	idata->used = true;
	idata->data_size = attr_size;
	idata->mempool = LIB_memory_pool_create(sizeof(float) * idata->data_size, 0, 16, 0);

	ROSE_assert(attr_size > 0);

	/* Push to linked list. */
	if (idatalist->idata_head[attr_size - 1] == NULL) {
		idatalist->idata_head[attr_size - 1] = idata;
	}
	else {
		idatalist->idata_tail[attr_size - 1]->next = idata;
	}
	idatalist->idata_tail[attr_size - 1] = idata;

	return idata;
}

static void DRW_instance_data_free(DRWInstanceData *idata) {
	LIB_memory_pool_destroy(idata->mempool);
}

void *DRW_instance_data_next(DRWInstanceData *idata) {
	return LIB_memory_pool_calloc(idata->mempool);
}

DRWInstanceData *DRW_instance_data_request(DRWInstanceDataList *idatalist, uint attr_size) {
	ROSE_assert(attr_size > 0 && attr_size <= MAX_INSTANCE_DATA_SIZE);

	DRWInstanceData *idata = idatalist->idata_head[attr_size - 1];

	/* Search for an unused data chunk. */
	for (; idata; idata = idata->next) {
		if (idata->used == false) {
			idata->used = true;
			return idata;
		}
	}

	return draw_instance_data_create(idatalist, attr_size);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Instance Data List (DRWInstanceDataList)
 * \{ */

DRWInstanceDataList *DRW_instance_data_list_create(void) {
	DRWInstanceDataList *idatalist = MEM_callocN(sizeof(DRWInstanceDataList), "DRWInstanceDataList");

	idatalist->pool_batching = LIB_memory_block_create(sizeof(GPUBatch *));
	idatalist->pool_instancing = LIB_memory_block_create(sizeof(DRWTempInstancingHandle));
	idatalist->pool_buffers = LIB_memory_block_create(sizeof(DRWTempBufferHandle));

	LIB_addtail(&GInstanceDataList, idatalist);

	return idatalist;
}

void DRW_instance_data_list_free(DRWInstanceDataList *idatalist) {
	DRWInstanceData *idata, *next_idata;

	for (int i = 0; i < MAX_INSTANCE_DATA_SIZE; i++) {
		for (idata = idatalist->idata_head[i]; idata; idata = next_idata) {
			next_idata = idata->next;
			DRW_instance_data_free(idata);
			MEM_freeN(idata);
		}
		idatalist->idata_head[i] = NULL;
		idatalist->idata_tail[i] = NULL;
	}

	LIB_memory_block_destroy(idatalist->pool_buffers, (MemblockValFreeFP)temp_buffer_handle_free);
	LIB_memory_block_destroy(idatalist->pool_instancing, (MemblockValFreeFP)temp_instancing_handle_free);
	LIB_memory_block_destroy(idatalist->pool_batching, (MemblockValFreeFP)temp_batch_free);

	LIB_remlink(&GInstanceDataList, idatalist);

	MEM_freeN(idatalist);
}

void DRW_instance_data_list_reset(DRWInstanceDataList *idatalist) {
	DRWInstanceData *idata;

	for (int i = 0; i < MAX_INSTANCE_DATA_SIZE; i++) {
		for (idata = idatalist->idata_head[i]; idata; idata = idata->next) {
			idata->used = false;
		}
	}
}

void DRW_instance_data_list_free_unused(DRWInstanceDataList *idatalist) {
	DRWInstanceData *idata, *next_idata;

	/* Remove unused data blocks and sanitize each list. */
	for (int i = 0; i < MAX_INSTANCE_DATA_SIZE; i++) {
		idatalist->idata_tail[i] = NULL;
		for (idata = idatalist->idata_head[i]; idata; idata = next_idata) {
			next_idata = idata->next;
			if (idata->used == false) {
				if (idatalist->idata_head[i] == idata) {
					idatalist->idata_head[i] = next_idata;
				}
				else {
					/* idatalist->idata_tail[i] is guaranteed not to be null in this case. */
					idatalist->idata_tail[i]->next = next_idata;
				}
				DRW_instance_data_free(idata);
				MEM_freeN(idata);
			}
			else {
				if (idatalist->idata_tail[i] != NULL) {
					idatalist->idata_tail[i]->next = idata;
				}
				idatalist->idata_tail[i] = idata;
			}
		}
	}
}

void DRW_instance_data_list_resize(DRWInstanceDataList *idatalist) {
	DRWInstanceData *idata;

	for (int i = 0; i < MAX_INSTANCE_DATA_SIZE; i++) {
		for (idata = idatalist->idata_head[i]; idata; idata = idata->next) {
			LIB_memory_pool_clear(idata->mempool, LIB_memory_pool_length(idata->mempool));
		}
	}
}

/** \} */
