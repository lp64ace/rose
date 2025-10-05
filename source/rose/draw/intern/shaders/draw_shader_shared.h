#ifndef DRAW_SHADER_SHARED_H
#define DRAW_SHADER_SHARED_H

#ifndef GPU_SHADER
#	include "GPU_shader_shared_utils.h"
#endif

#ifndef GPU_SHADER
typedef struct ObjectMatrices ObjectMatrices;
#endif

#define DRW_RESOURCE_CHUNK_LEN 512

struct ObjectMatrices {
	float4x4 drw_modelMatrix;
	float4x4 drw_modelMatrixInverse;
};

#define resource_id drw_ResourceID

#endif
