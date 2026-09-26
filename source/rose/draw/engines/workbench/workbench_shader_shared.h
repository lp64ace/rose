#ifndef WORKBENCH_SHADER_SHARED_H
#define WORKBENCH_SHADER_SHARED_H

#ifndef GPU_SHADER
#	include "GPU_shader_shared_utils.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct WorldData {
	float4 viewport_size;
	float4 object_outline_color;
	float4 shadow_direction_vs;

	float shadow_focus;
	float shadow_shift;
	float shadow_mul;
	float shadow_add;
};

#define viewport_size_inv viewport_size.zw

#ifdef __cplusplus
}
#endif

#endif	// !WORKBENCH_SHADER_SHARED_H