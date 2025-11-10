#ifndef DRAW_SHADER_SHARED_H
#define DRAW_SHADER_SHARED_H

#ifndef GPU_SHADER
#	include "GPU_shader_shared_utils.h"
#endif

#ifndef GPU_SHADER
typedef struct ObjectMatrices ObjectMatrices;
#endif

#define DRW_RESOURCE_CHUNK_LEN 256
#define DRW_RESOURCE_BONES_LEN 256

struct ObjectMatrices {
	float4x4 drw_modelMatrix;
	float4x4 drw_modelMatrixInverse;
	float4x4 drw_armatureMatrix;
	float4x4 drw_armatureMatrixInverse;
};

struct DVertGroupMatrices {
	float4x4 drw_poseMatrix;
};

#define resource_id drw_ResourceID

#endif
