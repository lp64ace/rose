#include "GPU_shader.h"

#include "alice_private.h"

typedef struct DRWAliceShaderList {
	GPUShader *depth;
} DRWAliceShaderList;

static DRWAliceShaderList GAliceShaderList;

GPUShader *DRW_alice_shader_depth_get(void) {
	if (GAliceShaderList.depth == NULL) {
		GAliceShaderList.depth = GPU_shader_create_from_info_name("alice_depth_mesh");
	}
	return GAliceShaderList.depth;
}

void DRW_alice_shaders_free() {
	GPUShader **shader_array = (GPUShader **)&GAliceShaderList;
	for (size_t index = 0; index < sizeof(DRWAliceShaderList) / sizeof(GPUShader *); index++) {
		if (shader_array[index]) {
			GPU_shader_free(shader_array[index]);
			shader_array[index] = NULL;
		}
	}
}
