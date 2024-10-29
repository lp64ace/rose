#include "gpu_test.hh"

#include <stdio.h>

namespace rose::gpu {

void GPUTest::SetUp() {
	manager = WTK_window_manager_new();
	ASSERT_NE(manager, nullptr);
	window = WTK_create_window(manager, "gpu::test", 800, 600);
	ASSERT_NE(window, nullptr);

	WTK_window_make_context_current(window);
	context = GPU_context_create(window, NULL);
	ASSERT_NE(context, nullptr);
}
void GPUTest::TearDown() {
	GPU_context_discard(context);
	WTK_window_free(manager, window);
	WTK_window_manager_free(manager);
}

} // namespace rose::gpu
