#ifndef OVERLAY_SHADER_SHARED_H
#define OVERLAY_SHADER_SHARED_H

#ifndef GPU_SHADER
#	include "GPU_shader_shared_utils.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct WorldData {
	float4 viewport_size;
	float4 object_outline_color;
};

#define viewport_size_inv viewport_size.zw

#ifdef __cplusplus
}
#endif

#endif	// !OVERLAY_SHADER_SHARED_H