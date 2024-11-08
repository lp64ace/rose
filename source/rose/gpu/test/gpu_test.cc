#include "LIB_assert.h"

#include "gpu_test.hh"

#include <stdio.h>

namespace rose::gpu {

void GPUTest::SetUp() {
	manager = WTK_window_manager_new();
	if (!manager) {
		GTEST_SKIP() << "Skipping gpu::test, GUI not supported.";
	}

	window = WTK_create_window(manager, "gpu::test", 800, 450);
	if (!window) {
		GTEST_SKIP() << "Skipping gpu::test, GUI not supported.";
	}

	WTK_window_make_context_current(window);
	WTK_window_hide(window);

	context = GPU_context_create(window, NULL);
	if (!context) {
		GTEST_SKIP() << "Skipping gpu::test, Graphics not supported.";
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

}  // namespace rose::gpu
