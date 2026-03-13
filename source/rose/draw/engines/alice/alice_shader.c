#include "LIB_string.h"

#include "GPU_shader.h"

#include "alice_private.h"

typedef struct DRWAliceShaderList {
	GPUShader *opaque;

	struct GPUShader *shadow_pass[2];
	struct GPUShader *shadow_fail[2][2];
} DRWAliceShaderList;

static DRWAliceShaderList GAliceShaderList;

ROSE_INLINE GPUShader *draw_alice_shader_shadow_pass_get_ex(bool depth, bool manifold, bool cap) {
	DRWAliceShaderList *list = &GAliceShaderList;

	struct GPUShader **shader = (depth) ? &list->shadow_pass[manifold] : &list->shadow_fail[manifold][cap];
	if (*shader == NULL) {
		const char *d = (depth) ? "pass" : "fail";
		const char *m = (manifold) ? "manifold" : "no_manifold";
		const char *c = (cap) ? "caps" : "no_caps";

		char name[64];
		LIB_strnformat(name, ARRAY_SIZE(name), "alice_shadow_%s_%s_%s", d, m, c);
		*shader = GPU_shader_create_from_info_name(name);
	}

	return *shader;
}

GPUShader *DRW_alice_shader_opaque_get(void) {
	if (GAliceShaderList.opaque == NULL) {
		GAliceShaderList.opaque = GPU_shader_create_from_info_name("alice_opaque_mesh");
	}
	return GAliceShaderList.opaque;
}

GPUShader *DRW_alice_shader_shadow_pass_get(bool manifold) {
	return draw_alice_shader_shadow_pass_get_ex(true, manifold, false);
}

GPUShader *DRW_alice_shader_shadow_fail_get(bool manifold, bool cap) {
	return draw_alice_shader_shadow_pass_get_ex(false, manifold, cap);
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
