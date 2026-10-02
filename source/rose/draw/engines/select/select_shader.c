#include "LIB_utildefines.h"

#include "select_private.h"

typedef struct DRWSelectShaderList {
	struct GPUShader *select_id_flat;
	struct GPUShader *select_id_uniform;
} DRWSelectShaderList;

static DRWSelectShaderList GSelectShaderList;

GPUShader *DRW_select_shader_id_flat_get(void) {
	if (!GSelectShaderList.select_id_flat) {
		GSelectShaderList.select_id_flat = GPU_shader_create_from_info_name("select_id_flat");
	}
	return GSelectShaderList.select_id_flat;
}

GPUShader *DRW_select_shader_id_uniform_get(void) {
	if (!GSelectShaderList.select_id_uniform) {
		GSelectShaderList.select_id_uniform = GPU_shader_create_from_info_name("select_id_uniform");
	}
	return GSelectShaderList.select_id_uniform;
}

void DRW_select_shaders_free() {
	GPUShader **shader_array = (GPUShader **)&GSelectShaderList;
	for (size_t index = 0; index < sizeof(GSelectShaderList) / sizeof(GPUShader *); index++) {
		if (shader_array[index]) {
			GPU_shader_free(shader_array[index]);
			shader_array[index] = NULL;
		}
	}
}
