#include "GPU_compute.h"

#include "gpu_backend.hh"

void GPU_compute_dispatch(struct GPUShader *shader, unsigned int groups_x_len, unsigned int groups_y_len, unsigned int groups_z_len) {
	rose::gpu::GPUBackend &backend = *rose::gpu::GPUBackend::get();
	GPU_shader_bind(shader);
	backend.compute_dispatch(groups_x_len, groups_y_len, groups_z_len);
}
