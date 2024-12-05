#ifndef USE_GPU_SHADER_CREATE_INFO
#	pragma once
#	include "GPU_shader_shared_utils.hh"
#endif

struct GPUClipPlanes {
	float4x4 ClipModelMatrix;
	float4 world[6];
};
