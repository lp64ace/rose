#ifndef DRAW_SHADER_SHARED_H
#define DRAW_SHADER_SHARED_H

#ifndef GPU_SHADER
#	include "GPU_shader_shared_utils.h"
#endif

#ifndef GPU_SHADER
typedef struct ObjectMatrices ObjectMatrices;
typedef struct ViewInfos ViewInfos;
#endif

#define DRW_RESOURCE_CHUNK_LEN 127
#define DRW_RESOURCE_BONES_LEN 127

struct ObjectMatrices {
	float4x4 drw_modelMatrix;
	float4x4 drw_modelMatrixInverse;
};

struct DVertGroupMatrices {
	float4x4 drw_TargetToArmature;
	float4x4 drw_ArmatureToTarget;
	float4x4 drw_poseMatrix[DRW_RESOURCE_BONES_LEN];
};

struct ViewInfos {
	/* View matrices */
	float4x4 winmat;
};

#ifdef USE_GPU_SHADER_CREATE_INFO
#	define ProjectionMatrix (drw_view.winmat)
#endif

#define resource_id drw_ResourceID

#endif
