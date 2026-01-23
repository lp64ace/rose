#include "GPU_shader.h"

#include "basic_private.h"

typedef struct DRWBasicShaderList {
	GPUShader *opaque;
	GPUShader *normal;
} DRWBasicShaderList;

static DRWBasicShaderList GBasicShaderList;

GPUShader *DRW_basic_shader_opaque_get(void) {
	if (GBasicShaderList.opaque == NULL) {
		GBasicShaderList.opaque = GPU_shader_create_from_info_name("basic_opaque_mesh");
	}
	return GBasicShaderList.opaque;
}

GPUShader *DRW_basic_shader_normal_get(void) {
	if (GBasicShaderList.normal == NULL) {
		GBasicShaderList.normal = GPU_shader_create_from_info_name("basic_normal_mesh");
	}
	return GBasicShaderList.normal;
}

void DRW_basic_shaders_free() {
	GPUShader **shader_array = (GPUShader **)&GBasicShaderList;
	for (size_t index = 0; index < sizeof(DRWBasicShaderList) / sizeof(GPUShader *); index++) {
		if (shader_array[index]) {
			GPU_shader_free(shader_array[index]);
			shader_array[index] = NULL;
		}
	}
}
