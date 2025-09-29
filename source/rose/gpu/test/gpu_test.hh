#pragma once

#include "MEM_guardedalloc.h"

#include "GPU_batch.h"
#include "GPU_batch_presets.h"
#include "GPU_batch_utils.h"
#include "GPU_common.h"
#include "GPU_common_types.h"
#include "GPU_compute.h"
#include "GPU_context.h"
#include "GPU_debug.h"
#include "GPU_drawlist.h"
#include "GPU_framebuffer.h"
#include "GPU_immediate.h"
#include "GPU_immediate_util.h"
#include "GPU_index_buffer.h"
#include "GPU_info.h"
#include "GPU_init_exit.h"
#include "GPU_material.h"
#include "GPU_matrix.h"
#include "GPU_platform.h"
#include "GPU_primitive.h"
#include "GPU_select.h"
#include "GPU_shader.h"
#include "GPU_shader_builtin.h"
#include "GPU_state.h"
#include "GPU_storage_buffer.h"
#include "GPU_texture.h"
#include "GPU_uniform_buffer.h"
#include "GPU_vertex_buffer.h"
#include "GPU_vertex_format.h"
#include "GPU_viewport.h"

#include "GTK_api.h"

#include "gtest/gtest.h"

namespace rose::gpu {

class GPUTest : public ::testing::Test {
	MEM_CXX_CLASS_ALLOC_FUNCS("GPUTest")
private:
	struct GTKWindowManager *manager = NULL;
	struct GTKWindow *window = NULL;

	GPUContext *context = NULL;

public:
	GPUTest() = default;

	void SetUp() override;
	void TearDown() override;
};

}  // namespace rose::gpu
