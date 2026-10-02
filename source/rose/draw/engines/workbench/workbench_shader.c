#include "LIB_string.h"

#include "GPU_shader.h"

#include "workbench_private.h"

typedef struct DRWWorkbenchShaderList {
	GPUShader *depth;
	GPUShader *opaque;
	GPUShader *outline;

	struct GPUShader *shadow_pass[2];
	struct GPUShader *shadow_fail[2][2];
} DRWWorkbenchShaderList;

static DRWWorkbenchShaderList GWorkbenchShaderList;

ROSE_INLINE GPUShader *draw_workbench_shader_shadow_pass_get_ex(bool depth, bool manifold, bool cap) {
	DRWWorkbenchShaderList *list = &GWorkbenchShaderList;

	struct GPUShader **shader = (depth) ? &list->shadow_pass[manifold] : &list->shadow_fail[manifold][cap];
	if (*shader == NULL) {
		const char *d = (depth) ? "pass" : "fail";
		const char *m = (manifold) ? "manifold" : "no_manifold";
		const char *c = (cap) ? "caps" : "no_caps";

		char name[64];
		const size_t n = LIB_strnformat(name, ARRAY_SIZE(name), "workbench_shadow_%s_%s_%s", d, m, c);
		ROSE_assert(n <= ARRAY_SIZE(name) - 1); // Increase the size of #name!
		*shader = GPU_shader_create_from_info_name(name);

		UNUSED_VARS_NDEBUG(n);
	}

	return *shader;
}

GPUShader *DRW_workbench_shader_depth_get(void) {
	if (GWorkbenchShaderList.depth == NULL) {
		GWorkbenchShaderList.depth = GPU_shader_create_from_info_name("workbench_depth_mesh");
	}
	return GWorkbenchShaderList.depth;
}

GPUShader *DRW_workbench_shader_opaque_get(void) {
	if (GWorkbenchShaderList.opaque == NULL) {
		GWorkbenchShaderList.opaque = GPU_shader_create_from_info_name("workbench_opaque_mesh");
	}
	return GWorkbenchShaderList.opaque;
}

GPUShader *DRW_workbench_shader_shadow_pass_get(bool manifold) {
	return draw_workbench_shader_shadow_pass_get_ex(true, manifold, false);
}

GPUShader *DRW_workbench_shader_shadow_fail_get(bool manifold, bool cap) {
	return draw_workbench_shader_shadow_pass_get_ex(false, manifold, cap);
}

GPUShader *DRW_workbench_shader_outline_get(void) {
	if (GWorkbenchShaderList.outline == NULL) {
		GWorkbenchShaderList.outline = GPU_shader_create_from_info_name("workbench_effect_outline");
	}
	return GWorkbenchShaderList.outline;
}

void DRW_workbench_shaders_free() {
	GPUShader **shader_array = (GPUShader **)&GWorkbenchShaderList;
	for (size_t index = 0; index < sizeof(DRWWorkbenchShaderList) / sizeof(GPUShader *); index++) {
		if (shader_array[index]) {
			GPU_shader_free(shader_array[index]);
			shader_array[index] = NULL;
		}
	}
}
