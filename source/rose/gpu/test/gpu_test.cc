#include "gpu_test.hh"

#include <stdio.h>

namespace rose::gpu {

void GPUTest::SetUp() {
	manager = WTK_window_manager_new();
	EXPECT_NE(manager, nullptr);
	window = WTK_create_window(manager, "gpu::test", 800, 600);
	EXPECT_NE(window, nullptr);

	WTK_window_make_context_current(window);
	context = GPU_context_create(window, NULL);
	EXPECT_NE(context, nullptr);
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
