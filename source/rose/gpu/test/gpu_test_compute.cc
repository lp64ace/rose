#include "gpu_test.hh"

#include "LIB_math_vector_types.hh"

#include <limits.h>

namespace rose::gpu {

TEST_F(GPUTest, ComputeDirect) {
	static unsigned int SIZE = 32;

	GPUShader *shader = GPU_shader_create_from_info_name("gpu_compute_2d_test");
	EXPECT_NE(shader, nullptr);

	GPUTexture *texture = GPU_texture_create_2d("gpu_shader_compute_2d", SIZE, SIZE, 1, GPU_RGBA32F, GPU_TEXTURE_USAGE_GENERAL, NULL);
	EXPECT_NE(texture, nullptr);

	GPU_shader_bind(shader);
	GPU_texture_image_bind(texture, GPU_shader_get_sampler_binding(shader, "canvas"));

	/* Dispatch compute task. */
	GPU_compute_dispatch(shader, SIZE, SIZE, 1);
	
	/* Check if compute has been done. */
	GPU_memory_barrier(GPU_BARRIER_TEXTURE_UPDATE);
	float4 *data = static_cast<float4 *>(GPU_texture_read(texture, GPU_DATA_FLOAT, 0));
	const float4 expected_result = float4(1.0, 0.5, 0.2, 1.0);
	EXPECT_NE(data, nullptr);
	for (unsigned int index = 0; index < SIZE * SIZE; index++) {
		EXPECT_FLOAT_EQ(data[index].x, expected_result.x);
		EXPECT_FLOAT_EQ(data[index].y, expected_result.y);
		EXPECT_FLOAT_EQ(data[index].z, expected_result.z);
		if (data[index] != expected_result) {
			/** Too verbose to print every single index! */
			break;
		}
	}
	MEM_freeN(data);

	GPU_shader_unbind();
	GPU_texture_unbind(texture);
	GPU_texture_free(texture);
	GPU_shader_free(shader);
}

} // namespace rose::gpu
