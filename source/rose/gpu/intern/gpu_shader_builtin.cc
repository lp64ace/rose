#include "LIB_assert.h"
#include "LIB_utildefines.h"

#include "GPU_info.h"
#include "GPU_shader.h"
#include "GPU_shader_builtin.h"

static GPUShader *builtin_shaders[GPU_SHADER_CFG_LEN][GPU_SHADER_BUILTIN_LEN] = {{nullptr}};

static const char *builtin_shader_create_info_name(BuiltinShader shader) {
	switch (shader) {
		case GPU_SHADER_TEXT:
			return "gpu_shader_text";
		case GPU_SHADER_2D_IMAGE_RECT_COLOR:
			return "gpu_shader_2D_image_rect_color";
		case GPU_SHADER_2D_IMAGE_OVERLAYS_MERGE:
			return "gpu_shader_2D_image_overlays_merge";
		case GPU_SHADER_2D_IMAGE_OVERLAYS_STEREO_MERGE:
			return "gpu_shader_2D_image_overlays_stereo_merge";
		case GPU_SHADER_2D_WIDGET_BASE:
			return "gpu_shader_2D_widget_base";
		case GPU_SHADER_2D_WIDGET_BASE_INST:
			return "gpu_shader_2D_widget_base_inst";
		case GPU_SHADER_2D_WIDGET_SHADOW:
			return "gpu_shader_2D_widget_shadow";

		case GPU_SHADER_TEST:
			return "gpu_shader_test";

		default:
			ROSE_assert_unreachable();
			return "";
	}
}

static const char *builtin_shader_create_info_name_clipped(BuiltinShader shader) {
	switch (shader) {
		default:
			ROSE_assert_unreachable();
			return "";
	}
}

GPUShader *GPU_shader_get_builtin_shader_with_config(BuiltinShader shader, ShaderConfig sh_cfg) {
	ROSE_assert(shader < GPU_SHADER_BUILTIN_LEN);
	ROSE_assert(sh_cfg < GPU_SHADER_CFG_LEN);
	GPUShader **sh_p = &builtin_shaders[sh_cfg][shader];

	if (*sh_p == nullptr) {
		if (sh_cfg == GPU_SHADER_CFG_DEFAULT) {
			/* Common case. */
			*sh_p = GPU_shader_create_from_info_name(builtin_shader_create_info_name(shader));
		}
		else if (sh_cfg == GPU_SHADER_CFG_CLIPPED) {
			/* In rare cases geometry shaders calculate clipping themselves. */
			*sh_p = GPU_shader_create_from_info_name(builtin_shader_create_info_name_clipped(shader));
		}
		else {
			ROSE_assert_unreachable();
		}
	}

	return *sh_p;
}

GPUShader *GPU_shader_get_builtin_shader(BuiltinShader shader) {
	return GPU_shader_get_builtin_shader_with_config(shader, GPU_SHADER_CFG_DEFAULT);
}

void GPU_shader_free_builtin_shaders() {
	for (int i = 0; i < GPU_SHADER_CFG_LEN; i++) {
		for (int j = 0; j < GPU_SHADER_BUILTIN_LEN; j++) {
			if (builtin_shaders[i][j]) {
				GPU_shader_free(builtin_shaders[i][j]);
				builtin_shaders[i][j] = nullptr;
			}
		}
	}
}
