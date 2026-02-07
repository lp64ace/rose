#include "MEM_guardedalloc.h"

#include "overlay_private.h"

typedef struct DRWOverlayShaderList {
	GPUShader *armature_shape_solid;
} DRWOverlayShaderList;

static DRWOverlayShaderList GOverlayShaderList;
static DRWOverlayInstanceFormats GOverlayInstanceFormats;

DRWOverlayInstanceFormats *DRW_overlay_shader_instance_formats() {
	if (GOverlayInstanceFormats.instance_bone == NULL) {
		GOverlayInstanceFormats.instance_bone = MEM_callocN(sizeof(GPUVertFormat), "DRWOverlayInstanceFormats::instance_bone");
		GPU_vertformat_add(GOverlayInstanceFormats.instance_bone, "InstanceModelMatrix", GPU_COMP_F32, 16, GPU_FETCH_FLOAT);
	}
	return &GOverlayInstanceFormats;
}

void DRW_overlay_shader_instance_formats_free() {
	GPUVertFormat **format_array = (GPUVertFormat **)&GOverlayInstanceFormats;
	for (size_t index = 0; index < sizeof(DRWOverlayInstanceFormats) / sizeof(GPUVertFormat *); index++) {
		if (format_array[index]) {
			MEM_SAFE_FREE(format_array[index]);
		}
	}
}

GPUShader *DRW_overlay_shader_armature_shape() {
	if (GOverlayShaderList.armature_shape_solid == NULL) {
		GOverlayShaderList.armature_shape_solid = GPU_shader_create_from_info_name("overlay_armature_shape_solid");
	}
	return GOverlayShaderList.armature_shape_solid;
}

void DRW_overlay_shaders_free() {
	GPUShader **shader_array = (GPUShader **)&GOverlayShaderList;
	for (size_t index = 0; index < sizeof(DRWOverlayShaderList) / sizeof(GPUShader *); index++) {
		if (shader_array[index]) {
			GPU_shader_free(shader_array[index]);
			shader_array[index] = NULL;
		}
	}
}
