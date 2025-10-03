#include "DRW_engine.h"

#define DRW_RESOURCE_CHUNK_LEN (1 << 8)

typedef struct DRWData {
	struct MemPool *commands;
	struct MemPool *passes;
	struct MemPool *shgroups;

	struct DRWViewData *vdata_engine[2];
} DRWData;

extern DRWManager GDrawManager;
