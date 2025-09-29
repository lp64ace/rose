#include "LIB_assert.h"

#include "gpu_test.hh"

#include <stdio.h>

namespace rose::gpu {

void GPUTest::SetUp() {
	manager = GTK_window_manager_new(GTK_WINDOW_MANAGER_NONE);
	if (!manager) {
		GTEST_SKIP() << "Skipping gpu::test, GUI not supported.";
	}

	window = GTK_create_window_ex(manager, "gpu::test", 800, 450, GTK_STATE_HIDDEN);
	if (!window) {
		GTEST_SKIP() << "Skipping gpu::test, GUI not supported.";
	}

	GTK_window_make_context_current(window);

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
		GTK_window_free(manager, window);
	}
	if (manager) {
		GTK_window_manager_free(manager);
	}
}

}  // namespace rose::gpu
