#include "LIB_utildefines.h"

#include "GPU_batch_presets.h"
#include "GPU_init_exit.h"
#include "GPU_shader_builtin.h"

#include "intern/gpu_private.h"

#include "intern/gpu_shader_create_info_private.hh"
#include "intern/gpu_shader_dependency_private.h"

/**
 * although the order of initialization and shutdown should not matter
 * (except for the extensions), I chose alphabetical and reverse alphabetical order
 */
static bool initialized = false;

void GPU_init() {
	/* can't avoid calling this multiple times, see wm_window_ghostwindow_add */
	if (initialized) {
		return;
	}

	initialized = true;

	gpu_shader_dependency_init();
	gpu_shader_create_info_init();

	gpu_batch_presets_init();
}

void GPU_exit() {
	gpu_batch_presets_exit();

	GPU_shader_free_builtin_shaders();

	gpu_shader_dependency_exit();
	gpu_shader_create_info_exit();

	gpu_backend_delete_resources();

	initialized = false;
}
