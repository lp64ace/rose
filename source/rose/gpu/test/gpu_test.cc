#include "LIB_assert.h"

#include "gpu_test.hh"

#include <stdio.h>

namespace rose::gpu {

void GPUTest::SetUp() {
	manager = WTK_window_manager_new();
	ROSE_assert_msg(manager != NULL, "[gpu::test] Failed to create window manager!");
	if (!manager) {
		return;
	}
	window = WTK_create_window(manager, "gpu::test", 800, 600);
	ROSE_assert_msg(window != NULL, "[gpu::test] Failed to create window!");
	if (!window) {
		return;
	}

	WTK_window_make_context_current(window);

	context = GPU_context_create(window, NULL);
	ROSE_assert_msg(window != NULL, "[gpu::test] Failed to create window context!");
	if (!context) {
		return;
	}
}
void GPUTest::TearDown() {
	if (context) {
		GPU_context_discard(context);
	}
	if (window) {
		WTK_window_free(manager, window);
	}
	if (manager) {
		WTK_window_manager_free(manager);
	}
}

} // namespace rose::gpu
