#ifndef DRAW_INSTANCE_DATA_H
#define DRAW_INSTANCE_DATA_H

#include "LIB_sys_types.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_INSTANCE_DATA_SIZE 64
#define DRW_BUFFER_VERTS_CHUNK 128

typedef struct DRWInstanceData DRWInstanceData;
typedef struct DRWInstanceDataList DRWInstanceDataList;
typedef struct DRWSparseUniformBuf DRWSparseUniformBuf;

/**
 * This manager allows to distribute existing batches for instancing
 * attributes. This reduce the number of batches creation.
 * Querying a batch is done with a vertex format. This format should
 * be static so that its pointer never changes (because we are using
 * this pointer as identifier [we don't want to check the full format
 * that would be too slow]).
 */
struct GPUVertBuf *DRW_temp_buffer_request(struct DRWInstanceDataList *idatalist, struct  GPUVertFormat *format, int *r_length);

/**
 * \note Does not return a valid drawable batch until DRW_instance_buffer_finish has run.
 * Initialization is delayed because instancer or geom could still not be initialized.
 */
struct GPUBatch *DRW_temp_batch_instance_request(struct DRWInstanceDataList *idatalist, struct GPUVertBuf *buffer, struct GPUBatch *instancer, struct GPUBatch *geometry);
/**
 * \note Use only with buf allocated via DRW_temp_buffer_request.
 */
struct GPUBatch *DRW_temp_batch_request(struct DRWInstanceDataList *idatalist, struct GPUVertBuf *buf, PrimType type);

/**
 * Upload all instance data to the GPU as soon as possible.
 */
void DRW_instance_buffer_finish(DRWInstanceDataList *idatalist);

void DRW_instance_data_list_reset(DRWInstanceDataList *idatalist);
void DRW_instance_data_list_free_unused(DRWInstanceDataList *idatalist);
void DRW_instance_data_list_resize(DRWInstanceDataList *idatalist);

#ifdef __cplusplus
}
#endif

#endif	// DRAW_INSTANCE_DATA_H